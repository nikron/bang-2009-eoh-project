/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implements "the master of slave peer threads" model.
 * */
#include"bang-com.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>

typedef struct _request_node {
	struct _request_node *next;
} request_node;

typedef struct {
	sem_t lock;
	sem_t num_requests;
	request_node *node;
} BANG_requests;

///Represents one of our peers.
typedef struct {
	pthread_t *thread;
	int peer_id;
	int socket;
	BANG_requests *requests;
} peer;

///A lock on both peers and num_peers, when you want to mess with one, you should want to
///mess with both.
sem_t peers_lock;

int num_peers = 0;
peer *peers = NULL;

/**
 * \page Peer Implemntation
 * Outgoing requests are serviced by iterating through the peers and sending out requests using the
 * peers list and corresponding structure.  There are two threads for each peer.  One to manage
 * incoming requests, and one to manage outgoing requests.
 */
void* BANG_peer_thread(void *peer_id) {
	while (1) {}
	return NULL;
}

void BANG_peer_added(int signal,int sig_id,void* socket) {
	BANG_add_peer(*((int*)socket));
	///TODO: socket currently leaks, fix this.
}

void BANG_add_peer(int socket) {
	///TODO: this should send out a peer_id in a non-leaking way.
	BANG_sigargs args;
	args.args = NULL;
	args.length = 0;
	BANG_send_signal(BANG_PEER_ADDED,args);
}

void BANG_peer_removed(int signal,int sig_id,void *peer_id) {
	BANG_remove_peer(*((int*)peer_id));
	///TODO: peer_id currently leaks, fix this.
}

void BANG_remove_peer(int peer_id) {
}

void BANG_com_init() {
	sem_init(&peers_lock,0,1);
}
