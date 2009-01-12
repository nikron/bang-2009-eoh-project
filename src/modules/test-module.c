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
#include<string.h>
#include"../base/bang-module-api.h"

char BANG_module_name[5] = "test";
unsigned char BANG_module_version[] = {0,0,1};
BANG_api api;

BANG_callbacks BANG_module_init(BANG_api get_api) {
	api = get_api;
	fprintf(stderr,"TEST:\t Module with name %s is initializing with version %d.%d.%d.\n",
			BANG_module_name,
			BANG_module_version[0],
			BANG_module_version[1],
			BANG_module_version[2]);
	BANG_callbacks callbacks;
	memset(&callbacks,0,sizeof(BANG_callbacks));
	return callbacks;
}

void BANG_module_run() {
	fprintf(stderr,"TEST:\t Module with name %s is running with version %d.%d.%d.\n",
			BANG_module_name,
			BANG_module_version[0],
			BANG_module_version[1],
			BANG_module_version[2]);
	api.BANG_debug_on_all_peers("TESTING ON ALL PEERS!\n");
}
