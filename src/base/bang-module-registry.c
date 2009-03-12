#include"bang-module.h"

void BANG_new_module(char *path, char **module_name, unsigned char **module_version) {
	
	BANG_module* new_mod = BANG_load_module(path);
	
	*module_name = new_mod->info->module_name;
	*module_version = new_mod->info->module_version;
	
}

BANG_module* BANG_get_module(char *module_name, unsigned char *module_version) {
	// TODO: this.
	module_name = NULL;
	module_version = NULL;
	return NULL;
}

int BANG_start_module(char *module_name, unsigned char *module_version) {
	// TODO: this as well.
	module_name = NULL;
	module_version = NULL;
	return 1;
}

void BANG_module_inform_new_peer(char *module_name, unsigned char *module_version, uuid_t new_peer) {
	module_name = NULL;
	module_version = NULL;
	new_peer = 0;
	// TODO: gotta call this, but need the params - BANG_module_new_peer(const BANG_module *module, uuid_t peer, uuid_t new_peer);
}
