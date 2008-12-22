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

void BANG_init(int *argc, char **argv) {
	int i = 0;
	char *port;
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
	BANG_sig_init();
	BANG_com_init();
	BANG_net_init(NULL,0);
}

void BANG_close() {
	fprintf(stderr,"BANG library closing.\n");
	BANG_com_close();
	BANG_net_close();
	BANG_sig_close();
}
