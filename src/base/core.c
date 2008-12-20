/**
 * \file core.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implememnts the main interface to the BANG project.  This is mainly accomplished
 * through use of BANG_init, BANG_close, and BANG_install_sighandler.
 */
#include"core.h"
#include"bang-net.h"
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>


///The port that will be passed to network thread.
char *port = DEFAULT_PORT;

///The network thread.
pthread_t *netthread;

void BANG_init(int *argc, char **argv) {
	int i = 0;
	for (i = 0; i < *argc; ++i) {
		if (!strcmp(argv[i],"--port")) {
			if (i + 1 < *argc) {
				printf("Port set to %s.\n",argv[i + 1]);
				port = argv[i + 1];
			} else {
				printf("Invalid port command.");
				exit(1);
			}
		}
	}
	netthread = (pthread_t*) malloc(sizeof(pthread_t));
	pthread_create(netthread,NULL,BANG_network_thread,(void*)port);
}

void BANG_close() {
	pthread_join(*netthread,NULL);
	free(netthread);
}
