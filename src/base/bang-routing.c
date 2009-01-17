#include"bang-routing.h"
#include<stdlib.h>
#include<uuid/uuid.h>


void BANG_register_module_route(BANG_module *module) {
	module->info->uuids = calloc(1,sizeof(uuid_t));
	uuid_generate(module->info->uuids[0]);
}
