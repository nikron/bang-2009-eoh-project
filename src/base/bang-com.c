/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implements "the master of slave peer threads" model.
 * */
#include"bang-com.h"
#include"bang-signals.h"
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
	BANG_requests *requests;
} peer;

int num_peers = 0;

peer *peers;

/**
 * \page Peer Implemntation
 * Outgoing requests are serviced by iterating through the peers and sending out requests using the
 * peers list and corresponding structure.  There are two threads for each peer.  One to manage
 * incoming requests, and one to manage outgoing requests.
 */
void* BANG_peer_thread(void *socket) {
	while (1) {}
	close(*((int*)socket));
	free(socket);
	return NULL;
}

void BANG_connection_signal_handler(int signal,int sig_id,void* socket) {
	peers= (peer*) realloc(peers,(++num_peers) * sizeof(peer));
	peers[num_peers - 1].thread = (pthread_t*) calloc(1,sizeof(pthread_t));
	peers[num_peers - 1].peer_id = num_peers - 1;
	peers[num_peers - 1].requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	sem_init(&(peers[num_peers - 1].requests->lock),0,1);
	sem_init(&(peers[num_peers - 1].requests->num_requests),0,0);
	pthread_create(peers[num_peers - 1].thread,NULL,&BANG_peer_thread,socket);
	BANG_acknowledge_signal(signal,sig_id);
}
