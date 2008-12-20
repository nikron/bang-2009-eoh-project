#include"core.h"
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int port = DEFAULT_PORT;
pthread_t *netthread;

void* network_thread(void *arg) {
	return NULL;
}

void BANG_init(int *argc, char **argv) {
	int i = 0;
	for (i = 0; i < *argc; ++i) {
		if (!strcmp(argv[i],"--port")) {
			if (i + 1 < *argc) {
				printf("Port set to %s.\n",argv[i + 1]);
				port = atoi(argv[i + 1]);
			} else {
				printf("Invalid port command.");
				exit(1);
			}
		}
	}
	netthread = (pthread_t*) malloc(sizeof(pthread_t));
	pthread_create(netthread,NULL,network_thread,NULL);
}

void BANG_close() {
	pthread_join(*netthread,NULL);
	free(netthread);
}
