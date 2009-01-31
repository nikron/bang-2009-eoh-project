#include"bang-peer.h"
#include"bang-peer-threads.h"
#include<stdlib.h>
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

BANG_peer* new_BANG_peer(int socket) {
	BANG_peer *new;

	new = (BANG_peer*) calloc(1,sizeof(BANG_peer));

	new->socket = socket;

	new->requests = new_BANG_requests();

	/* Set up the poll struct. */
	new->pfd.fd = socket;
	new->pfd.events = POLLIN  | POLLERR | POLLHUP | POLLNVAL;

	return new;
}

void free_BANG_peer(BANG_peer *p) {
#ifdef BDEBUG_1
	fprintf(stderr,"Freeing a peer with peer_id %d.\n",p->peer_id);
#endif

	free_BANG_requests(p->requests);

	close(p->socket);

	free(p);
}

void BANG_peer_set_peer_id(BANG_peer *p, int id) {
	p->peer_id = id;
}

void BANG_request_peer(BANG_peer *to_be_requested, BANG_request *request) {
	BANG_linked_list_push(to_be_requested->requests->requests,request);
	sem_post(&(to_be_requested->requests->num_requests));
}

void BANG_peer_start(BANG_peer *p) {
	pthread_create(&(p->receive_thread),NULL,BANG_read_peer_thread,p);
	pthread_create(&(p->send_thread),NULL,BANG_write_peer_thread,p);
}

void BANG_peer_stop(BANG_peer *p) {
	pthread_cancel(p->receive_thread);

	/* We need to close the receive thread in a more roundabout way, since it may be waiting
	 * on a semaphore in which case it will never close */
	BANG_request *request = new_BANG_request(BANG_CLOSE_REQUEST,NULL,0);

	BANG_request_peer(p,request);

	pthread_join(p->receive_thread,NULL);
	pthread_join(p->send_thread,NULL);
}

