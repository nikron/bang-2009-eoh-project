#include"bang-com.h"
#include"bang-routing.h"
#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-types.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
/* I am really reeling in those external libraries! */
#include<sqlite3.h>
#include<uuid/uuid.h>

#define DB_SCHEMA "CREATE TABLE mappings(route_uuid blob unique primary key, remote integer, peer_id int, module blob, name text, version blob)"
#define DB_FILE ":memory:"
#define REMOTE_ROUTE 2
#define LOCAL_ROUTE 1

static sqlite3 *db;

/* TODO: Many elements in the functions are the same, consolidate somehow. DRY, afterall*/

static sqlite3_stmt* prepare_select_statement(uuid_t uuid);

static void insert_route(uuid_t uuid, int remote, BANG_module *module, int peer_id, char *module_name, unsigned char *module_version);

static sqlite3_stmt* prepare_select_statement(uuid_t uuid) {
	assert(!uuid_is_null(uuid));

	sqlite3_stmt *get_peer_route;
	/* God dammit, ews needs to update sqlite */
#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,"SELECT remote,module,peer_id,name,version FROM mappings WHERE ? = route_uuid",90,&get_peer_route,NULL);
#else
	sqlite3_prepare(db,"SELECT remote,module,peer_id,name,version FROM mappings WHERE ? = route_uuid",90,&get_peer_route,NULL);
#endif

	sqlite3_bind_blob(get_peer_route,1,uuid,sizeof(uuid_t),SQLITE_STATIC);

	return get_peer_route;
}

void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));
	assert(job != NULL);

	/* Get the information about the peer, because we are routing to
	 * a peer. */
	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer.
			 * Make a non-shitty request.
			 * */
			BANG_request request;
			request.type = BANG_SEND_JOB_REQUEST;
			/* We are being a little presumptuous, and constructing the actual message
			 * that the communications infrastructure will send...
			 *
			 * Probably should move this to bang-com.c
			 */
			request.length = LENGTH_OF_HEADER +sizeof(uuid_t)  * 2 +
				LENGTH_OF_LENGTHS +
				4 /* A MAGIC NUMBER! */ +
				job->length;
			request.request = malloc(request.length);
			BANG_header header = BANG_SEND_JOB;

			memcpy(request.request,&header,LENGTH_OF_HEADER);
			memcpy(request.request,authority,sizeof(uuid_t));
			memcpy(request.request,peer,sizeof(uuid_t));
			memcpy(request.request,&(job->job_number),4);
			memcpy(request.request,&(job->length),LENGTH_OF_LENGTHS);
			memcpy(request.request,job->data,job->length);

			BANG_request_peer_id(sqlite3_column_int(get_peer_route,3),request);


		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_job(module,job,authority,peer);
		}
	}
}

/* COPY AND PASTE FUNCTIONS... must find better way.. */
void BANG_route_finished_job(uuid_t auth, uuid_t peer, BANG_job *job) {
	assert(!uuid_is_null(auth));
	assert(job != NULL);

	/* Get the information about the authority, because we are routing
	 * to an authority. */
	sqlite3_stmt *get_peer_route = prepare_select_statement(auth);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to remote peer. */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_job_finished(module,job,auth,peer);
		}
	}
}


void BANG_route_request_job(uuid_t peer, uuid_t authority) {
	/* We have to find where the authority, so we can request that
	 * authority to give us a job.
	 */

	/* Get the information about the authority, because we are routing
	 * to an authority. */
	sqlite3_stmt *get_auth_route = prepare_select_statement(authority);

	if (sqlite3_step(get_auth_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_auth_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer. */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_auth_route,2);
			BANG_module_callback_job_request(module,authority,peer);
		}
	}
}

void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer) {
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	/* Get the information about the peer, because we are routing
	 * to an authority. */
	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer. */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_job_available(module,authority,peer);
		}
	}
}

void BANG_route_new_peer(uuid_t peer, uuid_t new_peer) {
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(new_peer));

	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer.
			 * Not sure how we forward this... */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_new_peer(module,peer,new_peer);
		}
	}
}

void BANG_route_remove_peer(uuid_t peer, uuid_t old_peer) {
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(old_peer));

	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer.
			 * Not sure how we forward this... */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_remove_peer(module,peer,old_peer);
		}
	}
}

void BANG_route_send_message(uuid_t uuid, char *message) {
	assert(!uuid_is_null(uuid));
	assert(message != NULL);

	sqlite3_stmt *get_peer_route = prepare_select_statement(uuid);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer. */
		} else {
			/* Might as well print it out here, no reason to
			 * do stuipd contorions like making a call back
			 * function */
			fprintf(stderr,"%s",message);
		}
	}
}

int BANG_route_get_peer_id(uuid_t uuid) {
	assert(!uuid_is_null(uuid));

	sqlite3_stmt *get_peer_route = prepare_select_statement(uuid);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			return sqlite3_column_int(get_peer_route,3);
		}
	}

	return - 1;
}

int** BANG_not_route_get_peer_id(uuid_t *uuids) {
	if (uuids == NULL) return NULL;
	int i = 0, j = 0;
	int peer_id, **peer_ids = NULL;

	/* TODO: Make this actually construct a not list, not a list of
	 * the peers. */
	while (!uuid_is_null(uuids[i])) {
		sqlite3_stmt *select_statement = prepare_select_statement(uuids[i]);
		if (sqlite3_step(select_statement) == SQLITE_ROW &&
			(peer_id = sqlite3_column_int(select_statement,3)) != -1) {

			peer_ids = realloc(peer_ids,j++ + 1 * sizeof(int));
			peer_ids[j] = malloc(sizeof(int));
			*(peer_ids[j]) = peer_id;
		}
	}

	peer_ids[j + 1] = NULL;

	return peer_ids;
}

static void insert_route(uuid_t uuid, int remote, BANG_module *module, int peer_id, char *module_name, unsigned char *module_version) {
	assert(!uuid_is_null(uuid));
	assert(remote == LOCAL_ROUTE || remote == REMOTE_ROUTE);
	assert(peer_id >= 0);
	assert(module_name != NULL);
	assert(module_version != NULL);

	sqlite3_stmt *insert;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,"INSERT INTO mappings (route_uuid,remote,module,peer_id,name,text) VALUES (?,?,?,?,?)",85,&insert,NULL);
#else
	sqlite3_prepare(db,"INSERT INTO mappings (route_uuid,remote,module,peer_id,name,text) VALUES (?,?,?,?,?)",85,&insert,NULL);
#endif
	sqlite3_bind_blob(insert,1,uuid,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_bind_int(insert,2,remote);
	sqlite3_bind_blob(insert,3,module,sizeof(BANG_module*),SQLITE_STATIC);
	sqlite3_bind_int(insert,4,peer_id);
	sqlite3_bind_text(insert,5,module_name,-1,SQLITE_STATIC);
	sqlite3_bind_blob(insert,6,module_version,LENGTH_OF_VERSION,SQLITE_STATIC);

	sqlite3_step(insert);
	sqlite3_finalize(insert);
}

void BANG_register_module_route(BANG_module *module) {
	assert(module != NULL);

	/* MORE DEREFENCES THAN YOU CAN HANDLE!
	 * Create a uuid and insert into the module.
	 */
	module->info->peers_info->uuids = calloc(1,sizeof(uuid_t));
	module->info->my_id = 0;
	uuid_generate(module->info->peers_info->uuids[module->info->my_id]);
	module->info->peers_info->peer_number = 1;
	module->info->peers_info->validity[module->info->my_id] = 1;

	/* Insert the route in the database. */
	insert_route(module->info->peers_info->uuids[module->info->my_id],LOCAL_ROUTE,module,-1,module->module_name,module->module_version);
}


void BANG_register_peer_route(uuid_t uuid, int peer, char *module_name, unsigned char* module_version) {
	assert(!uuid_is_null(uuid));
	assert(peer >= 0);
	assert(module_name != NULL);
	assert(module_version != NULL);

	insert_route(uuid,REMOTE_ROUTE,NULL,peer,module_name,module_version);
}

void BANG_route_init() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG route initializing.\n");
#endif
	sqlite3_close(db);
}

void BANG_route_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG route closing.\n");
#endif
	/* Keep the mappings database in memory. */
#ifdef NEW_SQLITE
	sqlite3_open_v2(DB_FILE,&db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,NULL);
#else
	/* Dcl needs to update their sqlite3 libraries =/ */
	sqlite3_open(DB_FILE,&db);
#endif

	/*TODO: check for errors.
	 * Create database.
	 */
	sqlite3_exec(db,DB_SCHEMA,NULL,NULL,NULL);
}
