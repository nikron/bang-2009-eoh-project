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
	pthread_t receive_thread;
	pthread_t send_thread;
	int peer_id;
	int socket;
	BANG_requests *requests;
} peer;


sem_t peers_read_lock;
int readers = 0;
sem_t peers_change_lock;
///The total number of peers we get through the length of the program.
unsigned int peer_count = 0;

///The current number of peers connected to the program.
unsigned int current_peers = 0;
peer **peers = NULL;

///TODO: make this structure not take linear time when sending a request
///to a specific peer
int *keys = NULL;

/**
 * \page Peer Implementation
 * Outgoing requests are serviced by iterating through the peers and sending out requests using the
 * peers list and corresponding structure.  There are two threads for each peer.  One to manage
 * incoming requests, and one to manage outgoing requests.
 */

void peer_read_lock() {
	sem_wait(&peers_read_lock);
	if (readers == 0)
		sem_wait(&peers_change_lock);
	++readers;
	sem_post(&peers_read_lock);
}

void peer_read_unlock() {
	sem_wait(&peers_read_lock);
	--readers;
	if (readers == 0)
		sem_post(&peers_change_lock);
	sem_post(&peers_read_lock);
}

void* BANG_read_peer_thread(void *peer_info) {
	while (1) {}
	return NULL;
}

void* BANG_write_peer_thread(void *peer_info) {
	while(1) {}
	return NULL;
}

void BANG_peer_added(int signal,int sig_id,void* socket) {
	BANG_add_peer(*((int*)socket));
	free(socket);
}

int BANG_get_key_with_peer_id(int peer_id) {
	int i = 0;
	for (i = 0; i < current_peers; ++i) {
		if (peers[i]->peer_id == peer_id) {
			return i;
		}
	}

	return -1;
}

BANG_requests* allocateBANGRequests() {
	BANG_requests *requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	requests->node = NULL;
	sem_init(&(requests->lock),0,1);
	sem_init(&(requests->num_requests),0,0);
	return requests;
}

void BANG_add_peer(int socket) {
	sem_wait(&peers_change_lock);

	++current_peers;
	int current_key = current_peers - 1;
	int current_id = peer_count++;

	keys = (int*) realloc(keys,current_peers * sizeof(int));
	keys[current_key] = current_id;

	peers = (peer**) realloc(peers,current_peers * sizeof(peer*));
	peers[current_key] = (peer*) calloc(1,sizeof(peer));
	peers[current_key]->peer_id = current_id;
	peers[current_key]->socket = socket;
	peers[current_key]->requests = allocateBANGRequests();

	pthread_create(&(peers[current_key]->receive_thread),NULL,BANG_read_peer_thread,peers[current_key]);
	pthread_create(&(peers[current_key]->send_thread),NULL,BANG_write_peer_thread,peers[current_key]);

	sem_post(&peers_change_lock);

	//Send out that we successfully started the peer threads.
	BANG_sigargs args;
	args.args = calloc(1,sizeof(int));
	*((int*)args.args) = current_id;
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_ADDED,args);

	free(args.args);
}

void BANG_peer_removed(int signal,int sig_id,void *peer_id) {
	BANG_remove_peer(*((int*)peer_id));
	free(peer_id);
}

void BANG_remove_peer(int peer_id) {
	peer_read_lock();
	int pos = BANG_get_key_with_peer_id(peer_id);
	if (pos == -1) return;
	peer_read_unlock();
}

void BANG_com_init() {
	sem_init(&peers_change_lock,0,1);
	sem_init(&peers_read_lock,0,1);
}

///TODO: write this
void free_BANGRequests(BANG_requests *requests) {
}

void BANG_com_close() {
	///All of threads should of got hit by a global BANG_CLOSE_ALL
	///Should it be sent here?
	///Anyway, we'll just wait for each thread now
	int i = 0;
	for (i = 0; i < current_peers; ++i) {
		pthread_join(peers[i]->receive_thread,NULL);
		pthread_join(peers[i]->send_thread,NULL);
		free_BANGRequests(peers[i]->requests);
	}
	sem_destroy(&peers_change_lock);
	sem_destroy(&peers_change_lock);
}
