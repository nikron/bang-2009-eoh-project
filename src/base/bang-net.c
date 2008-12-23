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

///TODO:currently only one server can be run a time, good or bad?
///The server thread.
pthread_t *server_thread = NULL;
char *port = DEFAULT_PORT;
int sock = -1;

void free_server_addrinfo(void *result) {
	freeaddrinfo((struct addrinfo*)result);
}

void* BANG_server_thread(void *port) {
	//int sock; ///The main server socket
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

	///Let me rant about cleanup push.  First of all, it is a macro. WTF!?!?
	///Second of all it must have a matching pair pthread_cleanup_pop
	///Third of all, it means that anything between those two lines is
	///in a big do{}while(0).  Seriously. god damn.
	pthread_cleanup_push(free_server_addrinfo,result);

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
	int accptsock;
	while (1) {
		accptsock = accept(sock,rp->ai_addr,&rp->ai_addrlen);
		args.args = calloc(1,sizeof(int));
		*((int*)args.args) = accptsock;
		BANG_send_signal(BANG_PEER_CONNECTED,args);
		free(args.args);
	}

	pthread_cleanup_pop(1);
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

			///Sends out a signal of the peer with its socket.
			BANG_send_signal(BANG_PEER_CONNECTED,args);
			free(args.args);
			break;
		}
	}

	freeaddrinfo(result);
	return NULL;
}

void BANG_server_start(char *server_port) {
	if (server_port != NULL) {
		port = server_port;
	}
	pthread_create(server_thread,NULL,BANG_server_thread,(void*)port);
}


void BANG_server_stop() {
	pthread_cancel(*server_thread);
	pthread_join(*server_thread,NULL);
	close(sock);
}


void BANG_net_init(char *server_port ,char start_server) {
	if (server_port != NULL) {
		port = server_port;
	}

	server_thread = (pthread_t*) calloc(1,sizeof(pthread_t));
	if (start_server) {
		BANG_server_start(NULL);
	}
}

void BANG_net_close() {
	fprintf(stderr,"BANG net closing.\n");
	BANG_server_stop();
	free(server_thread);
	server_thread = NULL;
}
