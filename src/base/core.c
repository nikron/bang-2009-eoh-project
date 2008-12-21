/**
 * \file core.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief This implements the main public high level interface to the BANG project.  This is mainly accomplished
 * through use of BANG_init and BANG_close.
 */
#include"core.h"
#include"bang-net.h"
#include"bang-com.h"
#include"bang-signals.h"
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
	BANG_sig_init();
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
	BANG_install_sighandler(BANG_PEER_CONNECTED,&BANG_connection_signal_handler);
	netthread = (pthread_t*) malloc(sizeof(pthread_t));
	pthread_create(netthread,NULL,BANG_server_thread,(void*)port);
}

void BANG_close() {
	pthread_join(*netthread,NULL);
	free(netthread);
	BANG_sig_close();
}
