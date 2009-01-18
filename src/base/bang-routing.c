#include"bang-routing.h"
#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-types.h"
#include<stdlib.h>
/* I am really reeling in those external libraries! */
#include<sqlite3.h>
#include<uuid/uuid.h>

#define DB_SCHEMA "CREATE TABLE mappings(route_uuid blob unique primary key, remote integer, peer_id int, module blob, name text, version blob)"
#define REMOTE_ROUTE 2
#define LOCAL_ROUTE 1

static sqlite3 *db;

/* TODO: Many elements in the functions are the same, consolidate somehow. DRY, afterall*/

static sqlite3_stmt* prepare_select_statement(uuid_t uuid);

static sqlite3_stmt* prepare_select_statement(uuid_t uuid) {
	sqlite3_stmt *get_peer_route;
	sqlite3_prepare_v2(db,"SELECT remote,module,peer_id,name,version FROM mappings WHERE ? = route_uuid",90,&get_peer_route,NULL);
	return get_peer_route;
}

void BANG_route_job(uuid_t uuid, BANG_job *job) {
	sqlite3_stmt *get_peer_route = prepare_select_statement(uuid);
	sqlite3_bind_blob(get_peer_route,1,peer,sizeof(uuid_t),SQLITE_STATIC);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer. */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			/* TODO: Callback peer with job */
		}
	}
}

void BANG_register_module_route(BANG_module *module) {

	/* MORE DEREFENCES THAN YOU CAN HANDLE! */
	module->info->peers_info->uuids = calloc(1,sizeof(uuid_t));
	module->info->my_id = 0;
	uuid_generate(module->info->peers_info->uuids[module->info->my_id]);
	module->info->peers_info->peer_number = 1;
	module->info->peers_info->validity[module->info->my_id] = 1;

	sqlite3_stmt *insert_module;
	sqlite3_prepare_v2(db,"INSERT INTO mappings (route_uuid,remote,module,name,text) VALUES (?,1,?,?,?)",80,&insert_module,NULL);
	sqlite3_bind_blob(insert_module,1,module->info->peers_info->uuids[module->info->my_id],sizeof(uuid_t),SQLITE_STATIC);
	/* Store the _pointer_ to the module. */
	sqlite3_bind_blob(insert_module,2,module,sizeof(BANG_module*),SQLITE_STATIC);
	sqlite3_bind_text(insert_module,3,module->module_name,-1,SQLITE_STATIC);
	sqlite3_bind_blob(insert_module,4,module->module_version,LENGTH_OF_VERSION,SQLITE_STATIC);
	sqlite3_step(insert_module);
	sqlite3_finalize(insert_module);
}


void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer) {
	sqlite3_stmt *get_peer_route = prepare_select_statement(peer);
	sqlite3_bind_blob(get_peer_route,1,peer,sizeof(uuid_t),SQLITE_STATIC);

	if (sqlite3_step(get_peer_route) == SQLITE_ROW) {
		if (sqlite3_column_int(get_peer_route,1) == REMOTE_ROUTE) {
			/* TODO: Make a request to peer. */
		} else {
			const BANG_module *module = sqlite3_column_blob(get_peer_route,2);
			BANG_module_callback_jobs_available(module,authority,peer);
		}
	}
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
	sqlite3_open_v2(":memory",&db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,NULL);
	/*TODO: check for errors.
	 * Create database.
	 */
	sqlite3_exec(db,DB_SCHEMA,NULL,NULL,NULL);
}
