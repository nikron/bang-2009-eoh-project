/**
 * \file test-module.c
 * \author Nikhil Bysani
 */
#include<stdlib.h>
#include<stdio.h>

/**
 * \page Test Modules
 *
 * A bang module should be a C share object file.  Currently it must have
 * the following symbol names defined:
 *  -BANG_module_name: the name of the module
 *  -BANG_module_version: version of the module
 *  -BANG_module_init: initialize the module
 *  -BANG_module_run: runs the module
 */

char BANG_module_name[5] = "test";
double BANG_module_version = .1;

int BANG_module_init() {
	return 0;
}

void BANG_module_run() {
	fprintf(stderr,"TEST with a module name %s.\n",BANG_module_name);
}
