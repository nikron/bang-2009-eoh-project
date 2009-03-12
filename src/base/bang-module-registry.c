#include"bang-module.h"
#include"bang-utils.h"
#include"string.h"

static BANG_hashmap *modules = NULL;

typedef struct {
	BANG_module *module;
	char started;
} module_wrapper_t;

typedef struct {
	char *name;
	unsigned char *version;
} module_wrapper_key_t;

static unsigned int hash_module_wrapper_key_t(const void *data) {
	const module_wrapper_key_t *module_key = data;

	unsigned int hashcode;

	hashcode = strlen(module_key->name);
	hashcode |= module_key->name[hashcode/4] << 24;
	hashcode |= module_key->name[hashcode/2] << 12;
	hashcode |= module_key->version[0] << 4;
	hashcode |= module_key->version[1] << 8;

	return hashcode;
}

static int compare_module_wrapper_key_t(const void *d1, const void *d2) {
	const module_wrapper_key_t *mk1 = d1;
	const module_wrapper_key_t *mk2 = d2;

	return BANG_version_cmp(mk1->version, mk2->version) +
		strcmp(mk1->name, mk2->name);
}

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

void BANG_module_registry_init() {
	modules = new_BANG_hashmap(&hash_module_wrapper_key_t,&compare_module_wrapper_key_t);
}

void BANG_module_registry_close() {
}
