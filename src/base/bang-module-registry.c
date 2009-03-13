#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-utils.h"
#include"string.h"
#include<stdio.h>
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
	hashcode |= module_key->name[hashcode/4] << 8;
	hashcode |= module_key->version[0] << 16;
	hashcode |= module_key->version[1] << 24;

	return hashcode;
}


static int compare_module_wrapper_key_t(const void *d1, const void *d2) {
	const module_wrapper_key_t *mk1 = d1;
	const module_wrapper_key_t *mk2 = d2;

	int ret =  BANG_version_cmp(mk1->version, mk2->version) +
		strcmp(mk1->name, mk2->name);

#ifdef BDEBUG_1
	fprintf(stderr,"Found that %s and %s matched at %d.\n",mk1->name, mk2->name, ret);
	fprintf(stderr,"Version is at %p.\n",mk1->version);
#endif

	return ret;
}

void BANG_new_module(char *path, char **module_name, unsigned char **module_version) {
#ifdef BDEBUG_2
	fprintf(stderr,"Putting a new module in the registry.\n");
#endif

	BANG_module *module = BANG_load_module(path);

	if (module) {
		if (module_name) {
			*module_name = module->info->module_name;
		}

		if (module_version) {
			*module_version = module->info->module_version;
		}


		module_wrapper_t *module_wrapper = new_module_wrapper(module);
		module_wrapper_key_t module_key = create_key(module->info->module_name, module->info->module_version);

#ifdef BDEBUG_1
		fprintf(stderr,"Trying to put the %s module into a hashmap.\n",module_wrapper->module->info->module_name);
		fprintf(stderr,"With version:\t%p\n", module->info->module_version);
		fprintf(stderr,"With version:\t%d %d %d\n", module->info->module_version[0], module->info->module_version[1], module->info->module_version[2]);
#endif
		BANG_hashmap_set(modules,&module_key,module_wrapper);
	}
}

BANG_module* BANG_get_module(char *module_name, unsigned char *module_version) {
#ifdef BDEBUG_1
	fprintf(stderr,"Trying to get the %s module.\n", module_name);
#endif
	module_wrapper_key_t module_key = create_key(module_name, module_version);

	module_wrapper_t *module_wrapper = BANG_hashmap_get(modules,&module_key);

	if (module_wrapper)
		return module_wrapper->module;
	else
		return NULL;
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
#ifdef BDEBUG_1
	fprintf(stderr,"BANG module registry starting.\n");
#endif

	modules = new_BANG_hashmap(&hash_module_wrapper_key_t,&compare_module_wrapper_key_t);
}

void BANG_module_registry_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG module registry closing.\n");
#endif

	free_BANG_hashmap(modules);
}
