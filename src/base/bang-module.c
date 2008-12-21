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

BANG_module* BANG_load_module(char *path) {
	void *handle = dlopen(path,RTLD_NOW);
	char *check_errors = dlerror();
	if (check_errors != NULL) {
		BANG_send_signal(BANG_MODULE_ERROR,check_errors);
		return NULL;
	}

	BANG_module *module = (BANG_module*) calloc(1,sizeof(BANG_module));

	module->module_name = dlsym(handle,"module_name");
	if ((check_errors = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		BANG_send_signal(BANG_MODULE_ERROR,check_errors);
		return NULL;
	}

	*(void **) (&(module->module_init)) = dlsym(handle,"BANG_module_init");
	if ((check_errors = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		BANG_send_signal(BANG_MODULE_ERROR,check_errors);
		return NULL;
	}

	*(void **) (&(module->module_run)) = dlsym(handle,"BANG_module_run");
	if ((check_errors = dlerror()) != NULL) {
		dlclose(handle);
		free(module);
		BANG_send_signal(BANG_MODULE_ERROR,check_errors);
		return NULL;
	}

	return module;
}


void BANG_run_module(BANG_module *module) {
	BANG_send_signal(BANG_RUNNING_MODULE,module);
	module->module_init();
	module->module_run();
}
