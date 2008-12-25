/**
 * \file test-module.c
 * \author Nikhil Bysani
 * \date December 24, 2008
 *
 * \brief A simple test module to see if module functions are working
 * in order to flesh out a module api.
 */
#include<stdlib.h>
#include<stdio.h>
#include"../base/bang-module-api.h"

char BANG_module_name[5] = "test";
double BANG_module_version = .1;

int BANG_module_init() {
	fprintf(stderr,"TEST:\t Module with name %s is initializing with version %.1f.\n",BANG_module_name,BANG_module_version);
	return 0;
}

void BANG_module_run() {
	fprintf(stderr,"TEST:\t Module with name %s is running with version %.1f.\n",BANG_module_name,BANG_module_version);
	BANG_debug_on_all_peers("TESTING ON ALL PEERS!\n");
}
