/**
 * \file bang-net.c
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Implementation of the networking behind the BANG project.
 */
#include"bang-net.h"
#include"bang-signals.h"
#include<assert.h>
#include<errno.h>
#include<netdb.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>

/**
 * TODO:  currently only one server can be run a time, good or bad?
 */

/**
 * The server thread.
 */
static pthread_t *server_thread = NULL;
static char *port = DEFAULT_PORT;
static int sock = -1;
static pthread_mutex_t server_status_lock;

/*
 * Not being used because cleanup doesn't pass warnigns for some reason.
static void free_server_addrinfo(void *result) {
	freeaddrinfo((struct addrinfo*)result);
}
*/

void* BANG_server_thread(void *not_used) {
	not_used = NULL;
	assert(port != NULL);
	assert(sock == -1);

	struct addrinfo hints;
	struct addrinfo *result, *rp;
	BANG_sigargs args;

	/*
	 * sets the hints of getaddrinfo so it know what kind of address we want
	 * basic template of code from "man 2 getaddrinfo" section
	 *
	 * getaddrinfo is NOT C99, annoying to say the least.
	 */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	//check to see if we got available addresses
	if (getaddrinfo(NULL,port,&hints,&result) != 0) {
		BANG_send_signal(BANG_GADDRINFO_FAIL,NULL,0);
		return NULL;
	}

	/*
	 * Let me rant about cleanup push.  First of all, it is a macro. WTF!?!?
	 * Second of all it must have a matching pair pthread_cleanup_pop
	 * Third of all, it means that anything between those two lines is
	 * in a big do{}while(0).  Seriously. god damn.
	 */
	/*pthread_cleanup_push(free_server_addrinfo,result);*/

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (sock == -1) {
			/* TODO: Send something more meaningful */
			BANG_send_signal(BANG_BIND_FAIL,NULL,0);
		} else if (bind(sock,rp->ai_addr,rp->ai_addrlen) == 0) {
			/* TODO: Send something more meaningful */
			BANG_send_signal(BANG_BIND_SUC,NULL,0);
			break;
		}
	}

	/* check to see if we could bind to a socket */
	if (rp == NULL) {
		close(sock);
		/* TODO: Send something more meaningful */
		BANG_send_signal(BANG_BIND_FAIL,NULL,0);
		return NULL;
	}

	/* mark the socket for listening */
	if (listen(sock,MAX_BACKLOG) != 0) {
		close(sock);
		/* TODO: Send something more meaningful */
		BANG_send_signal(BANG_LISTEN_FAIL,NULL,0);
		return NULL;
	}

	BANG_send_signal(BANG_SERVER_STARTED,NULL,0);
	/* accepted client */
	args.length = sizeof(int);
	int accptsock;
	while (1) {
		accptsock = accept(sock,rp->ai_addr,&rp->ai_addrlen);
		args.args = calloc(1,sizeof(int));
		*((int*)args.args) = accptsock;
		BANG_send_signal(BANG_PEER_CONNECTED,&args,1);
		free(args.args);
	}


	/*pthread_cleanup_pop(1);*/
	close(sock);
	return NULL;
}

void* BANG_connect_thread(const void *addr) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	BANG_sigargs args;

	/*
	 * sets the hints of getaddrinfo so it know what kind of address we want
	 * basic template of code from "man 2 getaddrinfo" section
	 */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	//don't about ipv4 or ipv6
	hints.ai_socktype = SOCK_STREAM;//tcp
	hints.ai_flags = AI_PASSIVE;	//for wildcard IP address
	hints.ai_protocol = 0;		//any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	/* check to see if we got available addresses */
	if (getaddrinfo((char*)addr,port,&hints,&result) != 0) {
		args.args = (void*) addr;
		args.length = strlen(addr) * sizeof(char);
		/*
		 * TODO: Send something more meaningful
		 */
		BANG_send_signal(BANG_GADDRINFO_FAIL,&args,1);
		return NULL;
	}


	args.args = calloc(1,sizeof(int));
	args.length = sizeof(int);
#ifdef BDEBUG_1
	fprintf(stderr,"NET:\tConnect gaddrinfo has succeeded.\n");
#endif
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		*((int*)args.args) = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if (*((int*)args.args) == -1) {
			args.args = (void*) addr;
			args.length = strlen(addr) * sizeof(char);
			BANG_send_signal(BANG_CONNECT_FAIL,&args,1);

		} else if (connect(*((int*)args.args),rp->ai_addr,rp->ai_addrlen) == 0) {

			/*
			 * Sends out a signal of the peer with its socket.
			 */
#ifdef BDEBUG_1
			fprintf(stderr,"NET:\tA connect thread has succeeded.\n");
#endif
			BANG_send_signal(BANG_PEER_CONNECTED,&args,1);
			break;
		}
	}

	args.args = (void*) addr;
	args.length = strlen(addr) * sizeof(char);
	BANG_send_signal(BANG_CONNECT_FAIL,&args,1);

	freeaddrinfo(result);

	return NULL;
}

void BANG_server_start(char *server_port) {
	pthread_mutex_lock(&server_status_lock);
	if (server_port != NULL)
		port = server_port;

	if (server_thread == NULL) {
#ifdef BDEBUG_1
		fprintf(stderr,"Starting server.\n");
#endif
		server_thread = (pthread_t*) calloc(1,sizeof(pthread_t));
		pthread_create(server_thread,NULL,BANG_server_thread,NULL);
	}
	pthread_mutex_unlock(&server_status_lock);
}

void BANG_server_set_port(char *new_port) {
	pthread_mutex_lock(&server_status_lock);

	if (new_port != NULL && !strcmp(port,new_port)) {
		port = new_port;
	}

	pthread_mutex_unlock(&server_status_lock);
}

void BANG_server_stop() {
	pthread_mutex_lock(&server_status_lock);
	if (server_thread != NULL) {
#ifdef BDEBUG_1
		fprintf(stderr,"Stoping server.\n");
#endif
		pthread_cancel(*server_thread);
		pthread_join(*server_thread,NULL);
		if (sock != -1) {
			close(sock);
			sock = -1;
		}
		free(server_thread);
		server_thread = NULL;
		BANG_send_signal(BANG_SERVER_STOPPED,NULL,0);
	}
	pthread_mutex_unlock(&server_status_lock);
}

char BANG_is_server_running(){
	pthread_mutex_lock(&server_status_lock);
	if (server_thread) {
		pthread_mutex_unlock(&server_status_lock);
		return 1;
	}
	pthread_mutex_unlock(&server_status_lock);
	return 0;
}

void BANG_net_init(char *server_port ,char start_server) {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG net starting.\n");
#endif

	if (server_port != NULL) {
		port = server_port;
	}
	pthread_mutex_init(&server_status_lock,NULL);

	if (start_server) {
		BANG_server_start(NULL);
	}
}

void BANG_net_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG net closing.\n");
#endif
	BANG_server_stop();
	pthread_mutex_destroy(&server_status_lock);
}
