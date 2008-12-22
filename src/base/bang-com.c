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

///A linked list of requests of peer send threads.
typedef struct _request_node {
	///The next node in the list.
	struct _request_node *next;
} request_node;

/**
 * \return The head of the request node list starting at head.
 *
 * \brief Pops off the head of the linked list.
 */
request_node* pop_request(request_node **head);

/**
 * \param node appends node to list started at head
 * \param head start of the request node list
 *
 * \brief Appends a node to the head linked list.
 */
void append_request(request_node **head, request_node *node);

/**
 * \param head The head of the list to be freed.
 *
 * \brief Frees resources used by a request list started at
 * head.
 */
void free_requestList(request_node *head);

///Holds requests of the peers in a linked list.
typedef struct {
	///The lock on adding or removing requests.
	sem_t lock;
	///Signals the thread that a new requests has been added.
	sem_t num_requests;
	///A linked list of requests
	request_node *head;
} BANG_requests;

/*
 * \param requests The requests to be freed.
 *
 * \brief Frees a BANGRequests struct.
 */
void free_BANGRequests(BANG_requests *requests);

/*
 * \return Returns an initialized BANGRequest pointer.
 */
BANG_requests* allocate_BANGRequests();


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

request_node* pop_request(request_node  **head) {
	request_node *temp = *head;
	if (temp->next == NULL) {
		*head = NULL;
		return temp;

	} else {
		*head = temp->next;
		temp->next = NULL;
		return temp;
	}
}

void append_request(request_node **head, request_node *node) {
	if (*head == NULL) {
		*head = node;
	} else {
		request_node *cur;
		for (cur = *head; cur != NULL; cur = cur->next);
		cur->next = node;
	}
}

void* BANG_write_peer_thread(void *peer_info) {
	peer *self = (peer*) peer_info;
	request_node *current;
	while (1) {
		sem_wait(&(self->requests->num_requests));
		sem_wait(&(self->requests->lock));
		current = pop_request(&(self->requests->head));
		sem_post(&(self->requests->lock));

		///TODO: act on current request
	}
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

BANG_requests* allocate_BANGRequests() {
	BANG_requests *requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	requests->head = NULL;
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
	peers[current_key]->requests = allocate_BANGRequests();

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

void free_requestList(request_node *head) {
	if (head->next != NULL)
		free_requestList(head->next);
	free(head);
}

void free_BANGRequests(BANG_requests *requests) {
	sem_destroy(&(requests->lock));
	sem_destroy(&(requests->num_requests));
	free_requestList(requests->head);
}

void clear_peer(int pos) {
	///This should quickly make the two threads stop.
	///TODO:Send a close request to the send thread.
	close(peers[pos]->socket);
	pthread_join(peers[pos]->receive_thread,NULL);
	pthread_join(peers[pos]->send_thread,NULL);
	free_BANGRequests(peers[pos]->requests);
	free(peers[pos]);
}

void BANG_remove_peer(int peer_id) {
	sem_wait(&peers_change_lock);

	int pos = BANG_get_key_with_peer_id(peer_id);
	if (pos == -1) return;

	clear_peer(pos);

	for (;pos < current_peers - 1; ++pos) {
		peers[pos] = peers[pos + 1];
		keys[pos] = keys[pos + 1];
	}

	--current_peers;

	peers = (peer**) realloc(peers,current_peers * sizeof(peer*));
	keys = (int*) realloc(keys,current_peers * sizeof(int));

	sem_post(&peers_change_lock);
}

void BANG_com_init() {
	sem_init(&peers_change_lock,0,1);
	sem_init(&peers_read_lock,0,1);
}

void BANG_com_close() {
	///All of threads should of got hit by a global BANG_CLOSE_ALL
	///Should it be sent here?
	///Anyway, we'll just wait for each thread now
	int i = 0;
	sem_wait(&peers_change_lock);
	for (i = 0; i < current_peers; ++i) {
		clear_peer(i);
	}
	sem_destroy(&peers_change_lock);
	sem_destroy(&peers_read_lock);
	free(peers);
	free(keys);
	keys = NULL;
	peers = NULL;
	current_peers = 0;
	peer_count = 0;
}
