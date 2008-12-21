/**
 * \file bang-net.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implementation of the networking behind the BANG project.
 * */
#include"bang-net.h"
#include"bang-signals.h"
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<netdb.h>
#include<pthread.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>

void* BANG_server_thread(void *port) {
	int sock; ///The main server socket
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	//sets the hints of getaddrinfo so it know what kind of address we want
	//basic template of code from "man 2 getaddrinfo" section
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	//don't about ipv4 or ipv6
	hints.ai_socktype = SOCK_STREAM;//tcp
	hints.ai_flags = AI_PASSIVE;	//for wildcard IP address
	hints.ai_protocol = 0;		//any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	//check to see if we got available addresses
	if (getaddrinfo(NULL,(char*)port,&hints,&result) != 0) {
		BANG_send_signal(BANG_GADDRINFO_FAIL,NULL);
		return NULL;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (sock == -1) {
			BANG_send_signal(BANG_BIND_FAIL,NULL);
		} else if (bind(sock,rp->ai_addr,rp->ai_addrlen) == 0) {
			BANG_send_signal(BANG_BIND_SUC,NULL);
			break;
		}
	}

	//check to see if we could bind to a socket
	if (rp == NULL) {
		freeaddrinfo(result);
		BANG_send_signal(BANG_BIND_FAIL,NULL);
		return NULL;
	}

	//mark the socket for listening
	if (listen(sock,MAX_BACKLOG) != 0) {
		freeaddrinfo(result);
		BANG_send_signal(BANG_LISTEN_FAIL,NULL);
		return NULL;
	}

	//accepted client
	int *accptsock;
	while (1) {
		accptsock = (int*) calloc(1,sizeof(int));
		*accptsock = accept(sock,rp->ai_addr,&rp->ai_addrlen);
		BANG_send_signal(BANG_PEER_CONNECTED,accptsock);
	}

	freeaddrinfo(result);
	close(sock);
	return NULL;
}
