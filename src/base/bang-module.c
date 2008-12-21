/**
 * \file bang-module.c
 * \author Nikhil Bysani
 * \date December 21, 2009
 *
 * \brief A interface to load modules into the program.
 */
#include"bang-module.h"
#include"bang-signals.h"
#include"bang-types.h"
#include"bang-types.h"
#include<dlfcn.h>
#include<stdlib.h>
#include<string.h>

/**
 * \page BANG Modules
 *
 * A bang module should be a C share object file.  Currently it must have
 * the following symbol names defined:
 *  -module_name: the name of the module
 *  -BANG_module_init: initialize the module
 *  -Bang_module_run: runs the module
 */
BANG_module* BANG_load_module(char *path) {
	void *handle = dlopen(path,RTLD_NOW);
	BANG_sigargs args;
	args.args = dlerror();

	///Make sure the dlopen worked.
	if (args.args != NULL) {
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
		return NULL;
	}

	BANG_module *module = (BANG_module*) calloc(1,sizeof(BANG_module));

	module->module_name = dlsym(handle,"module_name");

	///Make sure the dlsym worked.
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
		return NULL;
	}

	*(void **) (&(module->module_init)) = dlsym(handle,"BANG_module_init");

	///Make sure the dlsym worked.
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
		return NULL;
	}

	*(void **) (&(module->module_run)) = dlsym(handle,"BANG_module_run");

	///Make sure the dlsym worked.
	if ((args.args = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		args.length = strlen(args.args);
		BANG_send_signal(BANG_MODULE_ERROR,args);
		free(args.args);
		return NULL;
	}

	return module;
}


void BANG_run_module(BANG_module *module) {
	BANG_sigargs args;
	args.args = module;
	args.length = sizeof(BANG_module);
	BANG_send_signal(BANG_RUNNING_MODULE,args);
	module->module_init();
	module->module_run();
}
