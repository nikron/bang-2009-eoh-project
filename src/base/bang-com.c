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
	close(*((int*)socket));
	return NULL;
}

///TODO: Fix this implemntaiton so that it uses other the functions ie so that
//it actually works
void BANG_connection_signal_handler(int signal,int sig_id,void* socket) {
	sem_wait(&peers_lock);
	int peer_id = ++num_peers;
	peers= (peer*) realloc(peers,(++num_peers) * sizeof(peer));
	sem_post(&peers_lock);

	peers[peer_id].thread = (pthread_t*) calloc(1,sizeof(pthread_t));
	peers[peer_id].peer_id = num_peers - 1;
	peers[peer_id].requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	sem_init(&(peers[peer_id].requests->lock),0,1);
	sem_init(&(peers[peer_id].requests->num_requests),0,0);
	pthread_create(peers[peer_id].thread,NULL,&BANG_peer_thread,peers + peer_id);
	BANG_acknowledge_signal(signal,sig_id);
}

void BANG_com_init() {
	sem_init(&peers_lock,0,1);
}
