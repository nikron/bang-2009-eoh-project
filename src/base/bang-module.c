/**
 * \file bang-module.c
 * \author Nikhil Bysani
 * \date December 21, 2008
 *
 * \brief A interface to load modules into the program.
 */
#include"bang-module.h"
#include"bang-com.h"
#include"bang-routing.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<assert.h>
#include<dlfcn.h>
#include<openssl/sha.h>
#include<openssl/evp.h>
#include<stdlib.h>
#include<string.h>

/**
 * \page BANG Modules
 *
 * A bang module should be a C share object file.  Currently it must have
 * the following symbol names defined:
 *  -BANG_module_name: the name of the module
 *  -BANG_module_version: version of the module
 *  -BANG_module_init: initialize the module
 *  -BANG_module_run: runs the module
 *
 *  How should the GUI interact with the module?  Using their owns symbols in the module
 *  such as "GUI_init".
 */

/**
 * \param path Valid path to a file.
 *
 * \brief Finds the sha1 hash of the file
 */
static unsigned char* module_hash(char *path) {
	assert(path != NULL);

	FILE *fd = fopen(path,"r");
	if (fd == NULL) return NULL;
	char buf[UPDATE_SIZE];
	int read = UPDATE_SIZE;

	unsigned char *md = (unsigned char*) calloc(SHA_DIGEST_LENGTH,sizeof(unsigned char));

	static EVP_MD_CTX *ctx = NULL;
	ctx = (ctx == NULL)  ? EVP_MD_CTX_create() : ctx;

	EVP_MD_CTX_init(ctx);
	/* TODO: Error checking! */
	EVP_DigestInit_ex(ctx,EVP_sha1(),NULL);


	/*
	 * We don't really need to do this.
	 * It'd be pretty interesting if we found a shared library bigger than memory.)
	 */
	while (read < UPDATE_SIZE) {
		read = fread(buf,sizeof(char),UPDATE_SIZE,fd);
		EVP_DigestUpdate(ctx,buf,read);
	}

	EVP_DigestFinal_ex(ctx,md,NULL);

	/* EVP_MD_CTX_destroy(ctx); */
	return md;
}

/**
 * \return An api.
 *
 * Allocates and returns an api for a module to use.  This memory
 * should not be touched by the module.
 */
static BANG_api* get_BANG_api() {
	static BANG_api *api = NULL;

	if (api == NULL) {
		/*If we forget to do something in this function, this will force
		 * the program to segfault when a module calls a function
		 * we were susposed to give them.
		 * calloc sets everything to zero*/

		/* This is memory will 'leak' we will let the OS clean it up, because
		 * the amount of memory leaked is constant and small */
		api = calloc(1,sizeof(BANG_api));

		api->BANG_debug_on_all_peers = &BANG_debug_on_all_peers;
		api->BANG_get_me_peers = &BANG_get_me_peers;
		api->BANG_number_of_active_peers = &BANG_number_of_active_peers;
		api->BANG_get_my_id = &BANG_get_my_id;
		api->BANG_assert_authority = &BANG_assert_authority;
		api->BANG_assert_authority_to_peer = &BANG_assert_authority_to_peer;
		api->BANG_request_job = &BANG_request_job;
		api->BANG_finished_request = &BANG_finished_request;
		api->BANG_send_job = &BANG_send_job;
	}

	return api;
}

/**
 * \param module_name The name of the module.
 * \param module_version The version of the module.
 *
 * \return A newly allocated BANG_module_info.
 *
 * \brief Creates a BANG_module_info for use by a module.
 */
BANG_module_info* new_BANG_module_info(char* module_name, unsigned char* module_version) {
	BANG_module_info *info = calloc(1,sizeof(BANG_module_info));

	info->peers_info = calloc(1,sizeof(peers_information));
	/* We aren't even a peer yet since we haven't been run/registered */
	info->peers_info->peer_number = 0;
	info->module_name = module_name;
	info->module_name_length = strlen(module_name);
	info->module_version = module_version;
	info->module_bang_name = malloc(info->module_name_length + 4);
	strcpy(info->module_bang_name, module_name);
	memcpy(info->module_bang_name + info->module_name_length + 1,module_version,3);

	info->my_id = 0;

	return info;
}

BANG_module* BANG_load_module(char *path) {
	assert(path != NULL);

	/* Get rid of any lingering errors. */
	/* while (dlerror() != NULL) {}
	 * Never mind, this apprently causes segfaults
	 */

	void *handle = dlopen(path,RTLD_NOW);

	char *module_name;
	unsigned char *module_version;

	BANG_sigargs args;
	args.args = dlerror();

	/* Make sure the dlsym worked. */
	if (args.args != NULL) {
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module.\n");
		fprintf(stderr,"%s",(char*)args.args);
		fprintf(stderr,"\n");
#endif
		return NULL;
	}

	BANG_module *module = (BANG_module*) calloc(1,sizeof(BANG_module));

	module_name = dlsym(handle,"BANG_module_name");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module name.\n");
#endif
		return NULL;
	}

	module_version = dlsym(handle,"BANG_module_version");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module version.\n");
#endif
		return NULL;
	}

	module->module_init = dlsym(handle,"BANG_module_init");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module init function.\n");
#endif
		return NULL;
	}

	module->module_run = dlsym(handle,"BANG_module_run");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module run function.\n");
#endif
		return NULL;
	}

	module->md = module_hash(path);
	/*
	 * The real question is, should I do a hash of the module handle?
	 * Then we need to know how long the module handle is.
	 */
	module->handle = handle;
	module->path = path;

#ifdef BDEBUG_1
	fprintf(stderr,"%s's init function has been called.\n",module_name);
#endif
	module->callbacks = module->module_init(get_BANG_api());
#ifdef BDEBUG_1
	fprintf(stderr,"%s's init function is over.\n",module_name);
#endif

	module->info = new_BANG_module_info(module_name, module_version);

#ifdef BDEBUG_1
	fprintf(stderr,"Created a new module, now returning it.\n");
#endif

	return module;
}

void BANG_unload_module(BANG_module *module) {
	assert(module != NULL);

	if (module != NULL) {
		dlclose(module->handle);
		/* TODO: This wont free it */
		free(module->info);
		free(module->md);
		free(module);
	}
}


void BANG_run_module(BANG_module *module) {
#ifdef BDEBUG_1
	fprintf(stderr,"Running module %s.\n", module->info->module_name);
#endif

	assert(module != NULL);

	if (module != NULL) {
		BANG_sigargs args;
		args.args = module;
		args.length = sizeof(BANG_module);
		BANG_send_signal(BANG_RUNNING_MODULE,&args,1);

		BANG_register_module_route(module);
		module->module_run(module->info);
	}
}

void* BANG_get_symbol(BANG_module *module, char *symbol) {
	assert(module != NULL);
	assert(symbol != NULL);
	char *error;

	if (module != NULL) {
		/* Get rid of any lingering errors. */
		while (dlerror() != NULL) {}

		void* sym_to_get =  dlsym(module->handle,symbol);

		if ((error = dlerror()) == NULL)
			return sym_to_get;
		else {
#ifdef BDEBUG_1
	fprintf(stderr,"Symbol %s returned with error %s.\n",symbol,error);
#endif
			return NULL;
		}
	} else
		return NULL;
}

static int BANG_module_info_peer_add(BANG_module_info *info, uuid_t new_peer) {
	int i;
	BANG_write_lock(info->lck);
	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (info->peers_info->validity[i] == 0) {
			uuid_copy(info->peers_info->uuids[i],new_peer);
			info->peers_info->validity[i] = 1;
			BANG_write_unlock(info->lck);

			return i;

		}
	}
	BANG_write_unlock(info->lck);
	
	return -1;
}

static int BANG_module_info_peer_remove(BANG_module_info *info, uuid_t old_peer) {
	int i;
	BANG_write_lock(info->lck);
	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (uuid_compare(info->peers_info->uuids[i],old_peer) == 0) {
			info->peers_info->validity[i] = 0;
			BANG_write_unlock(info->lck);

			return i;
		}
	}
	BANG_write_unlock(info->lck);
	i = info->peers_info->peer_number++;

	info->peers_info->uuids = realloc(info->peers_info->uuids,i * sizeof(uuid_t));
	info->peers_info->validity = realloc(info->peers_info->validity,i * sizeof(char));

	uuid_copy(info->peers_info->uuids[i - 1],old_peer);
	info->peers_info->validity[i - 1] = 1;

	return i - 1;
}

static char check_if_uuid_valid(BANG_module_info *info, uuid_t uuid, int id) {
	BANG_read_lock(info->lck);
	char valid = uuid_compare(info->peers_info->uuids[id],uuid) == 0;
	BANG_read_unlock(info->lck);

	return valid;
}

static int get_id_from_uuid(BANG_module_info *info, uuid_t uuid) {
	int i = 0;
	BANG_read_lock(info->lck);

	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (uuid_compare(info->peers_info->uuids[i],uuid) == 0) {
			BANG_read_unlock(info->lck);
			return i;
		}
	}

	BANG_read_unlock(info->lck);
	return -1;
}

void BANG_module_callback_job(const BANG_module *module, BANG_job *job, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	/* Make sure they have the right module. */
	if (check_if_uuid_valid(module->info,peer,module->info->my_id)) {
		job->peer = module->info->my_id;

		int id = get_id_from_uuid(module->info,auth);
		if (id != -1) {
			job->authority = id;
			module->callbacks->incoming_job(module->info,job);
		}
	}
}

void BANG_module_callback_job_available(const BANG_module *module, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	/* Make sure they have the right module. */
	if (check_if_uuid_valid(module->info,peer,module->info->my_id)) {

		int id = get_id_from_uuid(module->info,auth);
		if (id != -1) {
			module->callbacks->jobs_available(module->info,id);
		}
	}
}


void BANG_module_callback_job_request(const BANG_module *module, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	if (check_if_uuid_valid(module->info,auth,module->info->my_id)) {

		int id = get_id_from_uuid(module->info,peer);
		if (id != -1) {
			module->callbacks->outgoing_jobs(module->info,id);
		}
	}
}


void BANG_module_callback_job_finished(const BANG_module *module, BANG_job *job, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	if (check_if_uuid_valid(module->info,auth,module->info->my_id)) {
		job->authority = module->info->my_id;

		int id = get_id_from_uuid(module->info,peer);
		if (id != -1) {
			job->peer = id;
			module->callbacks->job_done(module->info,job);
		}
	}
}


void BANG_module_new_peer(const BANG_module *module, uuid_t peer, uuid_t new_peer) {
	assert(module != NULL);

	if (check_if_uuid_valid(module->info,peer,module->info->my_id)) {
		int id = BANG_module_info_peer_add(module->info, new_peer);
		module->callbacks->peer_added(module->info,id);
	}
}


void BANG_module_remove_peer(const BANG_module *module, uuid_t peer, uuid_t old_peer) {
	assert(module != NULL);

	if (check_if_uuid_valid(module->info,peer,module->info->my_id)) {
		int id = BANG_module_info_peer_remove(module->info, old_peer);
		module->callbacks->peer_removed(module->info,id);
	}
}

void BANG_get_uuid_from_local_id(uuid_t uuid, int id, BANG_module_info *info) {
	BANG_read_lock(info->lck);

	if (id < 0 || id > info->peers_info->peer_number ||
		info->peers_info->validity[id] == 0) {
		uuid_clear(uuid);
	} else {
		uuid_copy(uuid,info->peers_info->uuids[id]);
	}

	BANG_read_unlock(info->lck);
}
