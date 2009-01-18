#include"bang-routing.h"
#include<stdlib.h>
/* I am really reeling in those external libraries! */
#include<sqlite3.h>
#include<uuid/uuid.h>

#define DB_CREATE_STR "CREATE TABLE mappings(route_uuid blob unique primary key, remote integer, id integer, callbacks blob, name text, version text)"
#define REMOTE_ROUTE 2
#define LOCAL_ROUTE 1

static sqlite3 *db;

void BANG_register_module_route(BANG_module *module) {

	/* MORE DEREFENCES THAN YOU CAN HANDLE! */
	module->info->peers_info->uuids = calloc(1,sizeof(uuid_t));
	module->info->my_id = 0;
	uuid_generate(module->info->peers_info->uuids[module->info->my_id]);
	module->info->peers_info->peer_number = 1;
	module->info->peers_info->validity[module->info->my_id] = 1;

	sqlite3_stmt *insert_module;
	sqlite3_prepare_v2(db,"INSERT INTO mappings (route_uuid,remote,callbacks,name,text) VALUES (?,1,?,?,?)",73,&insert_module,NULL);
	sqlite3_bind_blob(insert_module,1,module->info->peers_info->uuids[module->info->my_id],sizeof(uuid_t),SQLITE_STATIC);
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
	sqlite3_open_v2(":memory",&db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,NULL);
	/*TODO: check for errors.
	 * Create database.
	 */
	sqlite3_exec(db,DB_CREATE_STR,NULL,NULL,NULL);
}
