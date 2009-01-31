/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Implements "the master of slave peer threads" model.
 */
#include"bang-com.h"
#include"bang-module-api.h"
#include"bang-peer-threads.h"
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

/*
 * \return Returns an initialized BANGRequest pointer.
 */
static BANG_requests* new_BANG_requests();

/*
 * \param requests The requests to be freed.
 *
 * \brief Frees a BANGRequests struct.
 */
static void free_BANG_requests(BANG_requests *requests);

static BANG_set *peers;

static BANG_requests* new_BANG_requests() {
	BANG_requests *new = (BANG_requests*) calloc(1,sizeof(BANG_requests));

	new->requests = new_BANG_linked_list();
	sem_init(&(new->num_requests),0,0);

	return new;
}

static void free_BANG_requests(BANG_requests *requests) {
	sem_destroy(&(requests->num_requests));
	free_BANG_linked_list(requests->requests,&free_BANG_request);
}

static void free_peer(BANG_peer *p) {
#ifdef BDEBUG_1
	fprintf(stderr,"Freeing a peer with peer_id %d.\n",p->peer_id);
#endif

	free_BANG_requests(p->requests);

	close(p->socket);

	free(p);
}

static BANG_peer* new_peer(int peer_id, int socket) {
	BANG_peer *new;

	new = (BANG_peer*) calloc(1,sizeof(BANG_peer));

	new->peer_id = peer_id;
	new->socket = socket;

	new->requests = new_BANG_requests();

	/* Set up the poll struct. */
	new->pfd.fd = socket;
	new->pfd.events = POLLIN  | POLLERR | POLLHUP | POLLNVAL;

	return new;
}

static void request_peer(BANG_peer *to_be_requested, BANG_request *request) {
	BANG_linked_list_push(to_be_requested->requests->requests,request);
	sem_post(&(to_be_requested->requests->num_requests));
}

static void peer_start(peer *p) {
	pthread_create(&(p->receive_thread),NULL,BANG_read_peer_thread,p);
	pthread_create(&(p->send_thread),NULL,BANG_write_peer_thread,p);
}

static void peer_stop(peer *p) {
	pthread_cancel(p->receive_thread);

	/* We need to close the receive thread in a more roundabout way, since it may be waiting
	 * on a semaphore in which case it will never close */
	BANG_request *request = new_BANG_request(BANG_CLOSE_REQUEST,NULL,0);

	request_peer(p,request);

	pthread_join(p->receive_thread,NULL);
	pthread_join(p->send_thread,NULL);
}

static peer_set* new_peer_set() {
	peer_set *new = calloc(1,sizeof(peer_set));

	new->lck = new_BANG_rw_syncro();
	new->peer_count = 0;
	new->current_peers = 0;
	new->size = 2;

	new->peers = calloc(new->size,sizeof(peers));
	new->keys = calloc(new->size,sizeof(int));

	return new;
}

static void free_peer_set(peer_set *ps) {
	unsigned int i = 0;

	BANG_write_lock(ps->lck);

	for (i = 0; i < ps->current_peers; ++i) {
		free_peer(ps->peers[i]);
	}

	BANG_write_unlock(ps->lck);

	free_BANG_rw_syncro(ps->lck);
	free(ps->keys);
	free(ps->peers);
	free(ps);
}

static int intcmp(const void *i1, const void *i2) {
	return *((int*) i1) - *((int*) i2);
}

static int internal_peer_set_get_key(peer_set *ps, int peer_id) {
	int pos = -1;

	int *ptr = bsearch(&peer_id,ps->keys,ps->current_peers,sizeof(int),&intcmp);
	if (ptr != NULL)
		pos = ps->keys - ptr;

	return pos;
}

static int peer_set_create_peer(peer_set *ps, int socket) {
	BANG_write_lock(ps->lck);

	int new_id = ++(ps->peer_count);
	int current_key = ++(ps->current_peers);

	peer *new = new_peer(new_id,socket);

	/* Grow the array if it is too small. */
	if (ps->size < ps->current_peers) {
		ps->size *= 2;

		ps->keys = realloc(ps->keys,ps->size * sizeof(int));
		ps->peers = realloc(ps->peers,ps->size * sizeof(peer*));
	}

	ps->keys[current_key] = new->peer_id;
	ps->peers[current_key] = new;

	BANG_write_unlock(ps->lck);

	return new_id;
}

static void peer_set_remove_peer_by_id(peer_set *ps, int peer_id) {
	BANG_write_lock(ps->lck);

	int pos = internal_peer_set_get_key(ps, peer_id);

	if (pos == -1) return;

	free_peer(ps->peers[pos]);
	ps->peers[pos] = NULL;

	for (;((unsigned int)pos) < ps->current_peers - 1; ++pos) {
		ps->peers[pos] = ps->peers[pos + 1];
		ps->keys[pos] = ps->keys[pos + 1];
	}

	--(ps->current_peers);

	BANG_write_unlock(ps->lck);
}

static void peer_set_start_peer_by_id(peer_set *ps, int peer_id) {
	BANG_read_lock(ps->lck);

	int key = internal_peer_set_get_key(ps, peer_id);
	peer_start(ps->peers[key]);

	BANG_read_unlock(ps->lck);
}

static void peer_set_stop_peer_by_id(peer_set *ps, int peer_id) {
	BANG_read_lock(ps->lck);

	int key = internal_peer_set_get_key(ps, peer_id);
	peer_stop(ps->peers[key]);

	BANG_read_unlock(ps->lck);
}

static peer* peer_set_get_peer(peer_set *ps, int peer_id) {
	peer *ret_peer = NULL;

	BANG_read_lock(ps->lck);

	int key = internal_peer_set_get_key(ps, peer_id);
	if (key != -1)
		ret_peer = ps->peers[key];

	BANG_read_unlock(ps->lck);

	return ret_peer;
}

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
	int peer_id = peer_set_create_peer(peers, socket);

#ifdef BDEBUG_1
	fprintf(stderr,"Threads being started at %d.\n",key);
#endif

	peer_set_start_peer_by_id(peers,peer_id);

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

	peer_set_stop_peer_by_id(peers,peer_id);
	peer_set_remove_peer_by_id(peers,peer_id);

	BANG_sigargs peer_send;
	peer_send.args = calloc(1,sizeof(int));
	*((int*)peer_send.args) = peer_id;
	peer_send.length = sizeof(int);
	BANG_send_signal(BANG_PEER_REMOVED,&peer_send,1);
	free(peer_send.args);
}

void BANG_request_peer_id(int peer_id, BANG_request *request) {
	peer *to_be_requested = peer_set_get_peer(peers,peer_id);
	request_peer(to_be_requested,request);
}

void BANG_request_all(BANG_request *request) {
	unsigned int i = 0;

	/* TODO: Do this a better a way, have a peer set function? */
	BANG_read_lock(peers->lck);

	for (; i < peers->current_peers; ++i) {
		request_peer(peers->peers[i],request);
	}

	BANG_read_unlock(peers->lck);
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
	free_peer_set(peers);
	peers = NULL;
}
