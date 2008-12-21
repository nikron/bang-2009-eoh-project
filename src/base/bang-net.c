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
	BANG_sigargs args;

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
		args.args = NULL;
		args.length = 0;
		BANG_send_signal(BANG_GADDRINFO_FAIL,args);
		return NULL;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (sock == -1) {
			args.args = NULL;
			args.length = 0;
			///TODO: Send something more meaningful.
			BANG_send_signal(BANG_BIND_FAIL,args);
		} else if (bind(sock,rp->ai_addr,rp->ai_addrlen) == 0) {
			args.args = NULL;
			args.length = 0;
			///TODO: Send something more meaningful
			BANG_send_signal(BANG_BIND_SUC,args);
			break;
		}
	}

	//check to see if we could bind to a socket
	if (rp == NULL) {
		freeaddrinfo(result);
		close(sock);
		args.args = NULL;
		args.length = 0;
		///TODO: Send something more meaningful
		BANG_send_signal(BANG_BIND_FAIL,args);
		return NULL;
	}

	//mark the socket for listening
	if (listen(sock,MAX_BACKLOG) != 0) {
		freeaddrinfo(result);
		close(sock);
		args.args = NULL;
		args.length = 0;
		///TODO: Send something more meaningful
		BANG_send_signal(BANG_LISTEN_FAIL,args);
		return NULL;
	}

	//accepted client
	args.length = sizeof(int);
	while (1) {
		args.args = calloc(1,sizeof(int));
		*((int*)args.args) = accept(sock,rp->ai_addr,&rp->ai_addrlen);
		BANG_send_signal(BANG_PEER_CONNECTED,args);
		free(args.args);
	}

	freeaddrinfo(result);
	close(sock);
	return NULL;
}


void* BANG_connect_thread(void *addr) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	BANG_sigargs args;

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
	if (getaddrinfo(NULL,(char*)addr,&hints,&result) != 0) {
		args.args = addr;
		args.length = strlen(addr) * sizeof(char);
		///TODO: Send something more meaningful
		BANG_send_signal(BANG_GADDRINFO_FAIL,args);
		free(args.args);
		return NULL;
	}


	args.args = calloc(1,sizeof(int));
	args.length = sizeof(int);
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		*((int*)args.args) = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (*((int*)args.args) == -1) {
			free(args.args);
			args.args = addr;
			args.length = strlen(addr) * sizeof(char);
			///TODO:Make this signal send out something more useful
			BANG_send_signal(BANG_CONNECT_FAIL,args);
			free(args.args);

		} else if (connect(*((int*)args.args),rp->ai_addr,rp->ai_addrlen) == 0) {

			BANG_send_signal(BANG_PEER_CONNECTED,args);
			free(args.args);
			break;
		}
	}

	freeaddrinfo(result);
	return NULL;
}
