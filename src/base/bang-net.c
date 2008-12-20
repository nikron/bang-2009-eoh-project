/**
 * \file bang-net.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implementation of the networking behind the BANG project.
 * */
#include"bang-net.h"
#include<stdio.h>
#include<errno.h>
#include<netdb.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>

void* BANG_network_thread(void *port) {
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
		perror("Could not get address info");

		/**
		 * TODO: Make this send out a signal before returning.
		 */
		return NULL;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (sock == -1) {
			/**
			 * TODO: Make this send out a signal before returning.
			 */
			perror("Bind failed");
		}
		else if (bind(sock,rp->ai_addr,rp->ai_addrlen) == 0) {
			/**
			 * TODO: Make this send out a signal before returning.
			 */
			printf("Bind succeeded.\n");
			fflush(stdout);
			break;
		}
	}

	//check to see if we could bind to a socket
	if (rp == NULL) {
		perror("Could not bind");
		freeaddrinfo(result);
		/**
		 * TODO: Make this send out a signal before returning.
		 */
		return NULL;
	}

	//mark the socket for listening
	if (listen(sock,MAX_BACKLOG) != 0) {
		perror("Could not open socket");
		freeaddrinfo(result);
		/**
		 * TODO: Make this send out a signal before returning.
		 */
		return NULL;
	}

	//accepted client
	int accptsock;
	while (1) {
		accptsock = accept(sock,rp->ai_addr,&rp->ai_addrlen);
		/**
		 * TODO: Make this send out a signal.
		 */
		shutdown(accptsock,SHUT_RDWR);
	}

	freeaddrinfo(result);
	return NULL;
}
