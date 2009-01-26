#include"bang-com.h"
#include"bang-routing.h"
#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-signals.h"
#include"bang-types.h"
#include"bang-utils.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
/* I am really reeling in those external libraries! */
#include<sqlite3.h>
#include<uuid/uuid.h>

#define CREATE_MAPPINGS "CREATE TABLE mappings(route_uuid blob unique primary key, remote integer, peer_id int, module blob, name text, version blob)"
#define INSERT_MAPPING "INSERT INTO mappings(route_uuid,remote,module,peer_id,name,text) VALUES (?,?,?,?,?)"
#define SELECT_MAPPING "SELECT remote,module,peer_id,name,version FROM mappings WHERE ? = route_uuid"
#define SELECT_UUID "SELECT route_uuid WHERE ? = peer_id"
#define REMOVE_MAPPING "DELETE FROM mappings WHERE ? = route_uuid"

#define CREATE_PEER_LIST "CREATE TABLE peers(id int)"
#define INSERT_PEER "INSERT INTO peers(id) VALUES (?)"
#define DELETE_PEER "DELETE FROM peers WHERE ? = id"

#define CREATE_ROUTES "CREATE TABLE routes(fst_peer blob, snd_peer blob)"
#define INSERT_ROUTE "INSERT INTO routes(fst_peer,snd_peer) VALUES (?,?)"
#define REMOVE_ROUTE "DELETE FROM routes WHERE (? = fst_peer AND ? = snd_peer) OR (? = fst_peer AND ? = snd_peer)"
#define SELECT_ROUTE "SELECT fst_peer, snd_peer FROM routes WHERE ? = fst_peer OR ? = snd_peer"

#define DB_FILE ":memory:"
#define REMOTE_ROUTE 2
#define LOCAL_ROUTE 1

/**
 * TODO: START ERROR CHECKING THE SQL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

static BANG_request construct_send_job_request(int type, uuid_t auth, uuid_t peer, int job_number, unsigned int job_length, void *data);

static void mem_append(void *dst, void *src, int length, int *pos);

static void insert_mapping(uuid_t uuid, int remote, BANG_module *module, int peer_id, char *module_name, unsigned char *module_version);

static void remove_mapping(uuid_t uuid);

static sqlite3_stmt* prepare_select_statement(uuid_t uuid);

static BANG_linked_list* select_routes_from_id(int id);

static void insert_route(uuid_t p1, uuid_t p2);

static void remove_route(uuid_t p1, uuid_t p2);

/**
 * \param p The peer you want to find a route of.
 *
 * \return A linked list of routes of the peer.
 */
static BANG_linked_list* select_route(uuid_t p);

void insert_peer(int id);

void remove_peer(int id);

static void catch_peer_added(int signal, int num_peers, void **p);

static void catch_peer_removed(int signal, int num_peers, void **p);

static sqlite3 *db;

/* TODO: Many elements in the functions are the same, consolidate somehow. DRY, afterall*/

void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));
	assert(job != NULL);

	/* Get the information about the peer, because we are routing to
	 * a peer. */
	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {

			BANG_request request =
				construct_send_job_request(
						BANG_SEND_JOB_REQUEST,
						authority,
						peer,
						job->job_number,
						job->length,
						job->data);

			BANG_request_peer_id(sqlite3_column_int(get_peer_route,3),request);


		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_job(module,job,authority,peer);
		}
	}
}

/* COPY AND PASTE FUNCTIONS... must find better way.. */
void BANG_route_finished_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(!uuid_is_null(authority));
	assert(job != NULL);

	/* Get the information about the authority, because we are routing
	 * to an authority. */
	sqlite3_stmt *get_auth_route = prepare_select_statement(authority);

	if (sqlite3_step(get_auth_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_auth_route,1) == REMOTE_ROUTE) {
			BANG_request request =
				construct_send_job_request(
						BANG_SEND_FINISHED_JOB_REQUEST,
						authority,
						peer,
						job->job_number,
						job->length,
						job->data);

			BANG_request_peer_id(sqlite3_column_int(get_auth_route,3),request);

		} else {
			const BANG_module *module = sqlite3_column_blob(get_auth_route,2);
			BANG_module_callback_job_finished(module,job,authority,peer);
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
			BANG_request request;
			request.type = BANG_SEND_REQUEST_JOB_REQUEST;
			request.length = sizeof(uuid_t) * 2;
			request.request = malloc(request.length);
			int pos = 0;
			mem_append(request.request,authority,sizeof(uuid_t),&pos);
			mem_append(request.request,peer,sizeof(uuid_t),&pos);

			BANG_request_peer_id(sqlite3_column_int(get_auth_route,3),request);

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
			BANG_request request;
			request.type = BANG_SEND_AVAILABLE_JOB_REQUEST;
			request.length = sizeof(uuid_t) * 2;
			request.request = malloc(request.length);
			int pos = 0;
			mem_append(request.request,authority,sizeof(uuid_t),&pos);
			mem_append(request.request,peer,sizeof(uuid_t),&pos);

			BANG_request_peer_id(sqlite3_column_int(get_peer_route,3),request);

		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_job_available(module,authority,peer);
		}
	}
}

void BANG_route_new_peer(uuid_t peer, uuid_t new_peer) {
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(new_peer));

	insert_route(peer,new_peer);

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

	remove_route(peer,old_peer);
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

void BANG_register_module_route(BANG_module *module) {
	assert(module != NULL);

	/* MORE DEFERENCES THAN YOU CAN HANDLE!
	 * Create a uuid and insert into the module.
	 */
	module->info->peers_info->uuids = calloc(1,sizeof(uuid_t));
	module->info->my_id = 0;
	uuid_generate(module->info->peers_info->uuids[module->info->my_id]);
	module->info->peers_info->peer_number = 1;
	module->info->peers_info->validity[module->info->my_id] = 1;

	/* Insert the route in the database. */
	insert_mapping(module->info->peers_info->uuids[module->info->my_id],LOCAL_ROUTE,module,-1,module->module_name,module->module_version);
}


void BANG_register_peer_route(uuid_t uuid, int peer, char *module_name, unsigned char* module_version) {
	assert(!uuid_is_null(uuid));
	assert(peer >= 0);
	assert(module_name != NULL);
	assert(module_version != NULL);

	insert_mapping(uuid,REMOTE_ROUTE,NULL,peer,module_name,module_version);
}

void BANG_deregister_route(uuid_t uuid) {
	assert(!uuid_is_null(uuid));

	remove_mapping(uuid);
}

void BANG_route_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG route closing.\n");
#endif

	sqlite3_close(db);
}

void BANG_route_init() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG route initializing.\n");
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
	sqlite3_exec(db,CREATE_MAPPINGS,NULL,NULL,NULL);
	sqlite3_exec(db,CREATE_PEER_LIST,NULL,NULL,NULL);
	sqlite3_exec(db,CREATE_ROUTES,NULL,NULL,NULL);

	BANG_install_sighandler(BANG_PEER_ADDED,&catch_peer_added);
	BANG_install_sighandler(BANG_PEER_REMOVED,&catch_peer_removed);
}

static void mem_append(void *dst, void *src, int length, int *pos) {
	memcpy(dst + *pos,src,length);
	*pos += length;
}

static BANG_request construct_send_job_request(int type, uuid_t auth, uuid_t peer, int job_number, unsigned int job_length, void *data) {
	BANG_request req;
	req.type = type;

	req.length = sizeof(uuid_t)  * 2 +
		LENGTH_OF_LENGTHS +
		4 /* A MAGIC NUMBER! */ +
		job_length;

	req.request = malloc(req.length);
	int pos = 0;

	mem_append(req.request,auth,sizeof(uuid_t),&pos);
	mem_append(req.request,peer,sizeof(uuid_t),&pos);
	mem_append(req.request,&(job_number),4,&pos);
	mem_append(req.request,&(job_length),LENGTH_OF_LENGTHS,&pos);
	mem_append(req.request,data,job_length,&pos);

	return req;
}

/*
 * SQL functions.
 * Functions dealing with the mappings table.
 */

static void insert_mapping(uuid_t uuid, int remote, BANG_module *module, int peer_id, char *module_name, unsigned char *module_version) {
	assert(!uuid_is_null(uuid));
	assert(remote == LOCAL_ROUTE || remote == REMOTE_ROUTE);
	assert(peer_id >= 0);
	assert(module_name != NULL);
	assert(module_version != NULL);

	sqlite3_stmt *insert;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,INSERT_MAPPING,-1,&insert,NULL);
#else
	sqlite3_prepare(db,INSERT_MAPPING,-1,&insert,NULL);
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

static void remove_mapping(uuid_t uuid) {
	assert(!uuid_is_null(uuid));

	sqlite3_stmt *remove;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,REMOVE_MAPPING,-1,&remove,NULL);
#else
	sqlite3_prepare(db,REMOVE_MAPPING,-1,&remove,NULL);
#endif

	sqlite3_bind_blob(remove,1,uuid,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_step(remove);
	sqlite3_finalize(remove);
}

static sqlite3_stmt* prepare_select_statement(uuid_t uuid) {
	assert(!uuid_is_null(uuid));

	sqlite3_stmt *get_peer_route;
	/* God dammit, ews needs to update sqlite */
#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,SELECT_MAPPING,-1,&get_peer_route,NULL);
#else
	sqlite3_prepare(db,SELECT_MAPPING,-1,&get_peer_route,NULL);
#endif

	sqlite3_bind_blob(get_peer_route,1,uuid,sizeof(uuid_t),SQLITE_STATIC);

	return get_peer_route;
}

static BANG_linked_list* select_routes_from_id(int id) {
	sqlite3_stmt *s_routes;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,SELECT_UUID,-1,&s_routes,NULL);
#else
	sqlite3_prepare(db,SELECT_UUID,-1,&s_routes,NULL);
#endif

	sqlite3_bind_int(s_routes,1,id);

	BANG_linked_list* list = new_BANG_linked_list();
	const void *temp;
	uuid_t *route;

	while (sqlite3_step(s_routes) == SQLITE_ROW) {
		temp = sqlite3_column_blob(s_routes,1);
		route = malloc(sizeof(uuid_t));
		uuid_copy(*route,*((uuid_t*)temp));

		BANG_linked_list_append(list,route);
	}

	return list;
}

/*
 * Functions dealing with the routes table.
 */

static void insert_route(uuid_t p1, uuid_t p2) {
	sqlite3_stmt *add_route;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,INSERT_ROUTE,-1,&add_route,NULL);
#else
	sqlite3_prepare(db,INSERT_ROUTE,-1,&add_route,NULL);
#endif

	sqlite3_bind_blob(add_route,1,p1,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_bind_blob(add_route,2,p2,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_step(add_route);
	sqlite3_finalize(add_route);
}

static void remove_route(uuid_t p1, uuid_t p2) {
	sqlite3_stmt *remove_route;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,REMOVE_ROUTE,-1,&remove_route,NULL);
#else
	sqlite3_prepare(db,REMOVE_ROUTE,-1,&remove_route,NULL);
#endif

	sqlite3_bind_blob(remove_route,1,p1,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_bind_blob(remove_route,2,p2,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_bind_blob(remove_route,3,p2,sizeof(uuid_t),SQLITE_STATIC);
	sqlite3_bind_blob(remove_route,4,p1,sizeof(uuid_t),SQLITE_STATIC);

	sqlite3_step(remove_route);
	sqlite3_finalize(remove_route);
}

BANG_linked_list* select_route(uuid_t p) {
	sqlite3_stmt *select_route;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,SELECT_ROUTE,-1,&select_route,NULL);
#else
	sqlite3_prepare(db,SELECT_ROUTE,-1,&select_route,NULL);
#endif

	sqlite3_bind_blob(select_route,1,p,sizeof(uuid_t),SQLITE_STATIC);

	const void *temp;
	uuid_t *route;
	BANG_linked_list *list = new_BANG_linked_list();

	while (sqlite3_step(select_route) == SQLITE_ROW) {
		temp = sqlite3_column_blob(select_route,1);

		if (uuid_compare(p,*((uuid_t*)temp)) == 0) {
			temp = sqlite3_column_blob(select_route,2);
		}

		uuid_copy(*route,*((uuid_t*)temp));

		BANG_linked_list_append(list,route);
	}

	sqlite3_finalize(select_route);

	return list;
}

void insert_peer(int id) {
	sqlite3_stmt *insert;
#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,INSERT_PEER,-1,&insert,NULL);
#else
	sqlite3_prepare(db,INSERT_PEER,-1,&insert,NULL);
#endif
	sqlite3_bind_int(insert,1,id);

	sqlite3_step(insert);
	sqlite3_finalize(insert);
}

void remove_peer(int id) {
	sqlite3_stmt *delete;

#ifdef NEW_SQLITE
	sqlite3_prepare_v2(db,DELETE_PEER,-1,&delete,NULL);
#else
	sqlite3_prepare(db,DELETE_PEER,-1,&delete,NULL);
#endif
	sqlite3_bind_int(delete,1,id);

	sqlite3_step(delete);
	sqlite3_finalize(delete);
}

/*
 * Catching the signals.
 */

static void catch_peer_added(int signal, int num_peers, void **p) {
	int **peers = (int**) p;

	if (signal == BANG_PEER_ADDED) {
		int i = 0;

		for (; i < num_peers; ++i) {
			insert_peer(*(peers[i]));
			free(peers[i]);
			peers[i] = NULL;
		}
	}

	free(peers);
}


static void catch_peer_removed(int signal, int num_peers, void **p) {
	int **peers = (int**) p;

	if (signal == BANG_PEER_ADDED) {
		int i;
		BANG_linked_list *lst, *route_list;
		uuid_t *route, *remote_route;

		for (i = 0; i < num_peers; ++i) {
			lst = select_routes_from_id(*(peers[i]));

			while ((route = BANG_linked_list_pop(lst)) != NULL) {
				route_list = select_route(*route);

				while ((remote_route = BANG_linked_list_pop(route_list)) != NULL) {
					BANG_route_remove_peer(*remote_route,*route);
					free(remote_route);
				}

				BANG_deregister_route(*route);
				free(route);
			}

			remove_peer(*(peers[i]));
			free(peers[i]);
			peers[i] = NULL;
		}
	}

	free(peers);
}
