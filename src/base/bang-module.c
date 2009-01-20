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

#define BDEBUG_1

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
 * \param module Modules to derive info from.
 * \return A newly allocated BANG_module_info.
 *
 * \brief Creates a BANG_module_info for use by a module.
 */
BANG_module_info* new_BANG_module_info() {
	BANG_module_info *info = calloc(1,sizeof(BANG_module_info));
	info->peers_info = calloc(1,sizeof(peers_information));
	/* We aren't even a peer yet since we haven't been run/registered */
	info->peers_info->peer_number = 0;

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
	BANG_sigargs args;
	args.args = dlerror();

	/* Make sure the dlsym worked. */
	if (args.args != NULL) {
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,&args,1);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module.\n");
		fprintf(stderr,args.args);
		fprintf(stderr,"\n");
#endif
		return NULL;
	}

	BANG_module *module = (BANG_module*) calloc(1,sizeof(BANG_module));

	module->module_name = dlsym(handle,"BANG_module_name");

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

	module->module_version = dlsym(handle,"BANG_module_version");

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

	module->callbacks = module->module_init(get_BANG_api());
	module->info = new_BANG_module_info();

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

	if (module != NULL) {
		/* Get rid of any lingering errors. */
		while (dlerror() != NULL) {}

		void* sym_to_get =  dlsym(module->handle,symbol);

		if (dlerror() == NULL)
			return sym_to_get;
		else
			return NULL;
	} else
		return NULL;
}

void BANG_module_callback_job_available(const BANG_module *module, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	/* Make sure they have the right module. */
	if (uuid_compare(module->info->peers_info->uuids[module->info->my_id],auth) == 0) {

		/* TODO: USE LOCKS! */
		int i = 0;
		for (i = 0; i < module->info->peers_info->peer_number; ++i) {
			if (uuid_compare(module->info->peers_info->uuids[i],peer) == 0) {
				module->callbacks->jobs_available(module->info,i);
			}
		}
	}
}


void BANG_module_callback_job_request(const BANG_module *module, uuid_t auth, uuid_t peer) {
	assert(module != NULL);

	/* Make sure they have the right module. */
	if (uuid_compare(module->info->peers_info->uuids[module->info->my_id],auth) == 0) {

		/* TODO: USE LOCKS! */
		int i = 0;
		for (i = 0; i < module->info->peers_info->peer_number; ++i) {
			if (uuid_compare(module->info->peers_info->uuids[i],peer) == 0) {
				module->callbacks->outgoing_job(module->info,i);
			}
		}
	}
}
