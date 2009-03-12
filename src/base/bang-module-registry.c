#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-utils.h"
#include"string.h"
#include<stdlib.h>
#include<pthread.h>
#include<uuid/uuid.h>

#define STARTED 1
#define STOPPED 0

typedef struct {
	BANG_module *module;
	char started;
	pthread_mutex_t lck;
} module_wrapper_t;

typedef struct {
	char *name;
	unsigned char *version;
} module_wrapper_key_t;

static BANG_hashmap *modules = NULL;

static module_wrapper_t* new_module_wrapper(BANG_module *module) {
	module_wrapper_t *new = malloc(sizeof(module_wrapper_t));

	pthread_mutex_init(&new->lck,NULL);
	new->module = module;
	new->started = STOPPED;

	return new;
}

static module_wrapper_key_t create_key(char *module_name, unsigned char *module_version) {
	module_wrapper_key_t module_key;

	module_key.name = module_name;
	module_key.version = module_version;

	return module_key;
}

static BANG_module* get_module_if_started_with_wrapper(module_wrapper_t *module_wrapper) {
	BANG_module *ret = NULL;

	if (module_wrapper != NULL) {
		pthread_mutex_lock(&module_wrapper->lck);

		ret = (module_wrapper->started == STARTED) ? module_wrapper->module : NULL;

		pthread_mutex_unlock(&module_wrapper->lck);
	}

	return ret;
}

static BANG_module* get_module_if_started(char *module_name, unsigned char *module_version) {
	module_wrapper_key_t module_key = create_key(module_name, module_version);

	module_wrapper_t *module_wrapper = BANG_hashmap_get(modules,&module_key);

	return get_module_if_started_with_wrapper(module_wrapper);
}

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

	if (module_name) {
		*module_name = new_mod->info->module_name;
	}

	if (module_version) {
		*module_version = new_mod->info->module_version;
	}

	module_wrapper_t *module_wrapper = new_module_wrapper(new_mod);
	module_wrapper_key_t module_key = create_key(new_mod->info->module_name, new_mod->info->module_version);

	/* TODO: THIS! */

	BANG_hashmap_set(modules,&module_key,module_wrapper);
}

BANG_module* BANG_get_module(char *module_name, unsigned char *module_version) {
	module_wrapper_key_t module_key = create_key(module_name, module_version);

	module_wrapper_t *module_wrapper = BANG_hashmap_get(modules,&module_key);

	return module_wrapper->module;
}

int BANG_run_module_in_registry(char *module_name, unsigned char *module_version) {
	int ret = -1;

	module_wrapper_key_t module_key = create_key(module_name, module_version);
	module_wrapper_t *module_wrapper = BANG_hashmap_get(modules,&module_key);

	pthread_mutex_lock(&module_wrapper->lck);

	if (module_wrapper->started == STOPPED) {
		BANG_run_module(module_wrapper->module);
		module_wrapper->started = STARTED;
		ret = 0;
	}

	pthread_mutex_unlock(&module_wrapper->lck);

	return ret;
}

void BANG_module_inform_new_peer(char *module_name, unsigned char *module_version, uuid_t new_peer) {
	BANG_module *module = get_module_if_started(module_name, module_version);

	if (module) {
		int module_id = BANG_get_my_id(module->info);
		uuid_t this_module_uuid;

		BANG_get_uuid_from_local_id(this_module_uuid, module_id, module->info);

		BANG_module_new_peer(module, this_module_uuid, new_peer);
	}
}

void BANG_module_registry_init() {
	modules = new_BANG_hashmap(&hash_module_wrapper_key_t,&compare_module_wrapper_key_t);
}

void BANG_module_registry_close() {
	free_BANG_hashmap(modules);
}
