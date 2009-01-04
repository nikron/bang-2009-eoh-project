/**
 * \file bang-module.c
 * \author Nikhil Bysani
 * \date December 21, 2008
 *
 * \brief A interface to load modules into the program.
 */
#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-signals.h"
#include"bang-types.h"
#include"bang-types.h"
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
 */

#define UPDATE_SIZE 1024

/**
 * \param path Valid path to a file.
 *
 * \brief Finds the sha1 hash of the file
 */
unsigned char* module_hash(char *path) {
	FILE *fd = fopen(path,"r");
	if (fd == NULL) return NULL;
	char buf[UPDATE_SIZE];
	int read = UPDATE_SIZE;

	unsigned char *md = (unsigned char*) calloc(SHA_DIGEST_LENGTH,sizeof(unsigned char));
	/*
	 * This ctx creation should be done in an init
	 * function as it takes some time.
	 */
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	EVP_MD_CTX_init(ctx);
	///TODO: Error checking!
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

	EVP_MD_CTX_destroy(ctx);
	return md;
}

BANG_module* BANG_load_module(char *path) {
	void *handle = dlopen(path,RTLD_NOW);
	BANG_sigargs args;
	args.args = dlerror();

	/* Make sure the dlsym worked. */
	if (args.args != NULL) {
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module.\n");
		fprintf(stderr,args.args);
		fprintf(stderr,"\n");
#endif
		free(args.args);
		return NULL;
	}

	BANG_module *module = (BANG_module*) calloc(1,sizeof(BANG_module));

	module->module_name = dlsym(handle,"BANG_module_name");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
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
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module version.\n");
#endif
		return NULL;
	}

	*(void **) (&(module->module_init)) = dlsym(handle,"BANG_module_init");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
#ifdef BDEBUG_1
		fprintf(stderr,"Could not find the module init function.\n");
#endif
		return NULL;
	}

	*(void **) (&(module->module_run)) = dlsym(handle,"BANG_module_run");

	/* Make sure the dlsym worked. */
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
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

	return module;
}

void BANG_unload_module(BANG_module *module) {
	if (module != NULL) {
		dlclose(module->handle);
		free(module->md);
		free(module);
	}
}


void BANG_run_module(BANG_module *module) {
	if (module != NULL) {
		BANG_api api;
		api.BANG_debug_on_all_peers = &BANG_debug_on_all_peers;
		BANG_sigargs args;
		args.args = module;
		args.length = sizeof(BANG_module);
		BANG_send_signal(BANG_RUNNING_MODULE,args);
		if(!module->module_init(&api)) {;
			module->module_run();
		}
	}
}
