#include"bang-routing.h"
#include<stdlib.h>
#include<uuid/uuid.h>


void BANG_register_module_route(BANG_module *module) {

	/* MORE DEREFENCES THAN YOU CAN HANDLE! */
	module->info->peers_info->uuids = calloc(1,sizeof(uuid_t));
	module->info->my_id = 0;
	uuid_generate(module->info->peers_info->uuids[module->info->my_id]);
	module->info->peers_info->peer_number = 1;
	module->info->peers_info->validity[module->info->my_id] = 1;

	/* TODO: Use locks, register to some global variable */
}
