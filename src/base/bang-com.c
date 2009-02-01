/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Implements "the master of slave peer threads" model.
 */
#include"bang-com.h"
#include"bang-module-api.h"
#include"bang-peer.h"
#include"bang-net.h"
#include"bang-routing.h"
#include"bang-signals.h"
#include"bang-types.h"
#include"bang-utils.h"
#include<poll.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>

static BANG_set *peers = NULL;

static void catch_add_peer(int signal, int num_sockets, void **socket) {
	if (signal == BANG_PEER_CONNECTED) {
		int **sock = (int**) socket;
		int i = 0;
		for (; i < num_sockets; ++i) {
			BANG_add_peer(*(sock[i]));
			free(sock[i]);
		}
		free(sock);
	}
}

static void catch_remove_peer(int signal, int num_peer_ids, void **peer_id) {
	if (signal == BANG_PEER_DISCONNECTED) {
		int **id = (int**) peer_id;
		int i = 0;
		for (; i < num_peer_ids; ++i) {
			BANG_remove_peer(*(id[i]));
			free(id[i]);
		}
		free(id);
	}
}

static void it_request_peer(void *p, void *req) {
	BANG_request_peer(p,req);
}

static void BANG_set_start_peer(BANG_set *s, int peer_id) {
	BANG_peer *p = BANG_set_get(s,peer_id);
	BANG_peer_start(p);
}

static int BANG_set_add_peer(BANG_set *s, int sock) {
	BANG_peer *new = new_BANG_peer(sock);
	int key = BANG_set_add(s,new);
	BANG_peer_set_peer_id(new,key);
	return key;
}

static void catch_request_all(int signal, int num_requests, void **vrequest) {
	if (signal == BANG_REQUEST_ALL) {
		BANG_request **to_request  = (BANG_request**) vrequest;
		int i = 0;
		for (; i < num_requests; ++i) {
			BANG_request_all(to_request[i]);
		}
		free(to_request);
	}
}

void BANG_add_peer(int socket) {
	int peer_id = BANG_set_add_peer(peers, socket);

#ifdef BDEBUG_1
	fprintf(stderr,"Threads being started at %d.\n",peer_id);
#endif

	BANG_set_start_peer(peers,peer_id);

	/**
	 * Send out that we successfully started the peer threads.
	 */
	BANG_sigargs args;
	args.args = calloc(1,sizeof(int));
	*((int*)args.args) = peer_id;
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_ADDED,&args,1);

	free(args.args);
}

void BANG_remove_peer(int peer_id) {
#ifdef BDEBUG_1
	fprintf(stderr,"Removing peer %d.\n",peer_id);
#endif

	BANG_peer *p = BANG_set_remove(peers,peer_id);
	BANG_peer_stop(p);

	BANG_sigargs peer_send;
	peer_send.args = calloc(1,sizeof(int));
	*((int*)peer_send.args) = peer_id;
	peer_send.length = sizeof(int);
	BANG_send_signal(BANG_PEER_REMOVED,&peer_send,1);
	free(peer_send.args);
}

void BANG_request_peer_id(int peer_id, BANG_request *request) {
	BANG_peer *to_be_requested = BANG_set_get(peers,peer_id);
	BANG_request_peer(to_be_requested,request);
}

void BANG_request_all(BANG_request *request) {
	BANG_set_iterate(peers,&it_request_peer,request);
}

void BANG_com_init() {
	BANG_install_sighandler(BANG_PEER_CONNECTED,&catch_add_peer);
	BANG_install_sighandler(BANG_PEER_DISCONNECTED,&catch_remove_peer);
	BANG_install_sighandler(BANG_REQUEST_ALL,&catch_request_all);
	peers = new_BANG_set();
}

void BANG_com_close() {
	/*
	 * All of threads should of got hit by a global BANG_CLOSE_ALL
	 * Should it be sent here?
	 * Anyway, we'll just wait for each thread now
	 */
#ifdef BDEBUG_1
	fprintf(stderr,"BANG com closing.\n");
#endif
	free_BANG_set(peers);
	peers = NULL;
}
