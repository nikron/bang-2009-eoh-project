/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implements "the master of slave peer threads" model.
 * 
 */
#include"bang-com.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
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


///The peers structure is a reader-writer structure.  That is, mulitple threads
///can be reading data from the structure, but only one process can change the
///size at once

///This is a lock on readers
sem_t peers_read_lock;

///The number of threads currently reading for the peers structure
int readers = 0;

///A lock on changing the size of peers
sem_t peers_change_lock;

///The total number of peers we get through the length of the program.
unsigned int peer_count = 0;

///The current number of peers connected to the program.
unsigned int current_peers = 0;
///Information and threads for each peer connected to the program
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

void clear_peer(int pos) {
	///This should quickly make the two threads stop.
	pthread_cancel(peers[pos]->receive_thread);
	pthread_cancel(peers[pos]->send_thread);
	///TODO: find a way to stop this thread.
	//pthread_join(peers[pos]->receive_thread,NULL);
	pthread_join(peers[pos]->send_thread,NULL);
	free_BANGRequests(peers[pos]->requests);
	close(peers[pos]->socket);
	free(peers[pos]);
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

BANG_requests* allocate_BANGRequests() {
	BANG_requests *requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	requests->head = NULL;
	sem_init(&(requests->lock),0,1);
	sem_init(&(requests->num_requests),0,0);
	return requests;
}

void free_BANGRequests(BANG_requests *requests) {
	sem_destroy(&(requests->lock));
	sem_destroy(&(requests->num_requests));
	free_requestList(requests->head);
}

///peer_info should be cleared by whoever by clear_peer
void* BANG_read_peer_thread(void *self_info) {
	//peer *self = (peer*)self_info;
	while (1) {}
	return NULL;
}

///peer_info should be cleared by whoever by clear_peer
void* BANG_write_peer_thread(void *self_info) {
	peer *self = (peer*)self_info;
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

void BANG_peer_added(int signal,int sig_id,void *socket) {
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

#ifdef BDEBUG_1
	fprintf(stderr,"Threads being started at %d\n",current_key);
#endif
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
	if (head == NULL) return;
	if (head->next != NULL)
		free_requestList(head->next);
	free(head);
}

void BANG_remove_peer(int peer_id) {
	///this lock is needed when trying to change the
	///peers structure
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
	BANG_install_sighandler(BANG_PEER_CONNECTED,&BANG_peer_added);
	sem_init(&peers_change_lock,0,1);
	sem_init(&peers_read_lock,0,1);
}

void BANG_com_close() {
	///All of threads should of got hit by a global BANG_CLOSE_ALL
	///Should it be sent here?
	///Anyway, we'll just wait for each thread now
	fprintf(stderr,"BANG com closing.\n");
	int i = 0;
	sem_wait(&peers_change_lock);
	for (i = 0; i < current_peers; ++i) {
		fprintf(stderr,"Clearing a peer at %d.\n",i);
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
