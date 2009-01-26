/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Implements "the master of slave peer threads" model.
 */
#include"bang-com.h"
#include"bang-module-api.h"
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

/**
 * Holds requests of the peers in a linked list.
 */
typedef struct {
	/**
	 * Signals the thread that a new requests has been added.
	 */
	sem_t num_requests;
	/**
	 * A linked list of requests
	 */
	BANG_linked_list *requests;
} BANG_requests;

/**
 * The peers structure is a reader-writer structure.  That is, multiple threads
 * can be reading data from the structure, but only one process can change the
 * size at once.
 */
typedef struct {
	/**
	 * The unique id of a peer that the client library will use.
	 */
	int peer_id;
	/**
	 * The name supplied by the peer.
	 */
	char *peername;
	/**
	 * The actual socket of the peer.
	 */
	int socket;
	/**
	 * The thread that is reading in from the peer.
	 */
	pthread_t receive_thread;
	/**
	 * The list of requests that the send thread will execute.
	 */
	BANG_requests *requests;
	/**
	 * Performs the requests in the requests list.
	 */
	pthread_t send_thread;
	/**
	 * Poll set up so that the peer wont have to reconstruct it at every step
	 * note: perhaps pass this around rather than put it in global space.
	 */
	struct pollfd pfd;
} peer;

/**
 * A structure to access information about peers.  It's purpose is to have atmoic addition, removal,
 * and lookup of peers.  Hopefully it should also have constant addition, removal, and lookup, but
 * it currently has constant addition, linear removal, and log(n) lookup.
 */
typedef struct {
	/**
	 * A lock on peers structure and affiliate things.  It can have multiple readers,
	 * but only one writer.
	 */
	BANG_rw_syncro *lck;

	unsigned int peer_count;
	unsigned int current_peers;
	size_t size;

	peer **peers;
	int *keys;
} peer_set;


/**
 * \param self The peer to be requested.
 * \param request Request to be given to the peer.
 *
 * \brief Adds a request to a peer structure.
 */
static void request_peer(peer *self, BANG_request *request);

/**
 * \param peer_id Id to be used for the peer.
 * \param socket Socket to be used for the peer.
 *
 * \brief Allocates and returns a new peer.
 */
static peer* new_peer(int peer_id, int socket);

/**
 * \brief Removes and frees a peer and its resources.
 */
static void free_peer(peer *p);

/**
 * \param p The peer to stop.
 *
 * \brief Stops a peer's thread.  Note:  This
 * will most likely make the remote end hang up.
 */
static void peer_stop(peer *p);

/**
 * \param p The peer to start.
 *
 * Starts a peer's threads.
 */
static void peer_start(peer *p);

/**
 * \return A new and allocated peer_set.
 */
static peer_set* new_peer_set();

/**
 * \brief Frees and dealloctes the peer set.
 */
static void free_peer_set();

/**
 * \param peer_id The id of a peer.
 *
 * \brief Gets the location of the peer in the in
 * the peers array.
 */
static int internal_peer_set_get_key(peer_set *ps, int peer_id);

/**
 * \param peers The set to add the peer to.
 * \param socket The socket of the new peer.
 *
 * \return The peer_id of the peer.
 *
 * \brief Adds new_peer to the peers array.
 */
static int peer_set_create_peer(peer_set *ps, int socket);

/**
 * \param ps The peer set to remove an peer from.
 * \param peer_id The peer to remove from the peer_set.
 *
 * \brief Removes peer_id from ps if it exisits.
 */
static void peer_set_remove_peer_by_id(peer_set *ps, int peer_id);

/**
 * \param ps The peer set with peer peer_id
 * \param peer_id A valid peer_id in ps
 *
 * \brief Calls peer_start on the peer peer_id.
 */
static void peer_set_start_peer_by_id(peer_set *ps, int peer_id);

/**
 * \param ps The peer set with peer peer_id
 * \param peer_id A valid peer_id in ps
 *
 * \brief Calls peer_stop on the peer peer_id.
 */
static void peer_set_stop_peer_by_id(peer_set *ps, int peer_id);

/**
 * \param ps The peer set with peer peer_id
 * \param peer_id A valid peer_id in ps
 *
 * \return Gets the peer peer_id from ps.
 *
 * \brief Gets the peer peer_id from ps.
 */
static peer* peer_set_get_peer(peer_set *ps, int peer_id);

/**
 * \param self The peer to be closed.
 *
 * \brief A peer thread asks to close itself.
 */
static void read_peer_thread_self_close(peer *self);

/**
 * \param self The peer wanting to extract a message from the socket.
 * \param length The length of the message to extract.
 *
 * \brief Extracts a message of length length.
 */
static void* extract_message(peer *self, unsigned int length);

static unsigned int write_message(peer *self, void *message, unsigned int length);

/**
 * \param self The peer responding to BANG_HELLO.
 *
 * \brief Acts on an incoming BANG_HELLO.
 */
static char peer_respond_hello(peer *self);

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

static peer_set *peers;

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

static void free_peer(peer *p) {
#ifdef BDEBUG_1
	fprintf(stderr,"Freeing a peer with peer_id %d.\n",p->peer_id);
#endif

	free_BANG_requests(p->requests);

	close(p->socket);

	free(p);
}

static peer* new_peer(int peer_id, int socket) {
	peer *new;

	new = (peer*) calloc(1,sizeof(peer));

	new->peer_id = peer_id;
	new->socket = socket;

	new->requests = new_BANG_requests();

	/* Set up the poll struct. */
	new->pfd.fd = socket;
	new->pfd.events = POLLIN  | POLLERR | POLLHUP | POLLNVAL;

	return new;
}

static void request_peer(peer *to_be_requested, BANG_request *request) {
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
	peers = new_peer_set();
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

/*
 * HERE THERE BE DRAGONS!
 *
 * Or, actual peer logic starts here...
 */

static char peer_respond_hello(peer *self) {
	unsigned char *version = (unsigned char*) extract_message(self,LENGTH_OF_VERSION);
	/* TODO: Make this more elegant =P */
	if (BANG_version_cmp(version,BANG_LIBRARY_VERSION) == 0) {
		free(version);
		return 0;
	}
	free(version);

	unsigned int *length = (unsigned int*) extract_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}
	self->peername = (char*) extract_message(self,*length);
	free(length);
	return 1;
}

static char read_debug_message(peer *self) {
	unsigned int *length = (unsigned int*) extract_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}
	char *message = (char*) extract_message(self,*length);
	if (message == NULL) {
		return 0;
	}
	fprintf(stderr,"%s",message);
	free(message);
	return 1;

}

static char read_module_message(peer *self) {
	unsigned int *length = (unsigned int*) extract_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}

	BANG_sigargs args;
	args.args = (char*) extract_message(self,*length);

	if (args.args == NULL) {
		free(length);
		return 0;
	}

	args.length = *length;
	free(length);

	BANG_send_signal(BANG_RECEIVED_MODULE,&args,1);
	free(args.args);
	return 1;
}

static void extract_uuid(peer *self,uuid_t uuid) {
	uuid_t *uuid_ptr = (uuid_t*) extract_message(self,sizeof(uuid_t));
	if (uuid_ptr) {
		uuid_copy(uuid,*uuid_ptr);
		free(uuid_ptr);
	} else {
		uuid_clear(uuid);
	}
}

static char read_job_message(peer *self) {
	uuid_t auth, peer;

	extract_uuid(self,auth);
	if (uuid_is_null(auth)) {
		return 0;
	}

	extract_uuid(self,peer);
	if (uuid_is_null(peer)) {
		return 0;
	}

	BANG_job job;

	/* A MAGIC NUMBER */
	int *job_number = (int*) extract_message(self,4);
	if (job_number == NULL) {
		return 0;
	}

	job.job_number = *job_number;
	free(job_number);

	unsigned int *job_length  = (unsigned int*) extract_message(self,LENGTH_OF_LENGTHS);
	if (job_length == NULL) {
		return 0;
	}

	job.length = *job_length;
	free(job_length);

	job.data = extract_message(self,job.length);
	if (job.data == NULL) {
		return 0;
	}

	BANG_route_job(auth,peer,&job);

	return 1;
}

/**
 * \param self The thread sending the module.
 * \param request The module path to send.
 *
 * \brief Sends a module to a peer.
 *
 * Discussion:  Would it be better to keep the module in memory, so
 * we don't have to read from disk every time we send a module?  However,
 * it may be possible that modules don't fit in memory though this seems
 * _very_ unlikely.
 */
static void send_module(peer *self, BANG_request *request) {
	FILE *fd = fopen((char*)request->request,"r");
	if (fd == NULL) {
		return;
	}

	char chunk[UPDATE_SIZE];
	size_t reading;
	unsigned int i;

	i = BANG_SEND_MODULE;
	/* TODO: Error checking */
	write_message(self,&i,LENGTH_OF_HEADER);

	/* TODO: Error checking */
	fseek(fd,0,SEEK_END);
	i = (unsigned int) ftell(fd);
	rewind(fd);
	write_message(self,&i,LENGTH_OF_LENGTHS);

	do {
		/* TODO: Error checking */
		reading = fread(chunk,UPDATE_SIZE,1,fd);
		write_message(self,chunk,reading);

	} while (reading >= UPDATE_SIZE);

	fclose(fd);
}

static void send_module_peer_request(peer *self, BANG_request *request) {
	BANG_header header = BANG_WANT_MODULE;
	write_message(self,&header,LENGTH_OF_HEADER);
	write_message(self,request->request,request->length);
}

static void read_peer_thread_self_close(peer *self) {
	BANG_sigargs args;
	args.args = &(self->peer_id);
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_DISCONNECTED,&args,1);

	pthread_exit(NULL);
}

static unsigned int write_message(peer *self, void *message, unsigned int length) {
	unsigned int written = 0;
	int write_return = 0;
	char writing = 1;
	while (writing) {
		write_return = write(self->socket,message,length);
		if (write_return >= 0) {
			written += write_return;
			if (written >= length)
				writing = 0;
		} else {
			writing = 0;
			/* ERROR !*/
		}
	}

	return written;
}

static void* extract_message(peer *self, unsigned int length) {
	void *message = (char*) calloc(length,1);
	int check_read;
	unsigned int have_read = 0;
	char extracting = 1;

	while (extracting) {
		if (poll(&(self->pfd),1,-1) != -1 && self->pfd.revents & POLLIN) {
			check_read = read(self->socket,message + have_read,length);

			if (check_read <= 0) {
#ifdef BDEBUG_1
				fprintf(stderr,"Read on the socket has returned an error.\n");
#endif

				free(message);
				message = NULL;
				extracting = 0;
			} else {
				have_read += check_read;
				extracting = (have_read >= length) ? 0 : 1;
			}
		} else {
#ifdef BDEBUG_1

			fprintf(stderr,"POLLIN:\t%x.\n",POLLIN);
			fprintf(stderr,"POLLOUT:\t%x.\n",POLLOUT);
			fprintf(stderr,"POLLERR:\t%x.\n",POLLERR);
			fprintf(stderr,"POLLHUP:\t%x.\n",POLLHUP);
			fprintf(stderr,"POLLNVAL:\t%x.\n",POLLNVAL);
			fprintf(stderr,"Something went wrong on poll, revents %x.\n",self->pfd.revents);
#endif
			free(message);
			message = NULL;
			extracting = 0;
		}
	}

	return message;
}

void* BANG_read_peer_thread(void *self_info) {
	peer *self = (peer*)self_info;

	BANG_header *header;

	char reading = 1;

	while (reading) {
		if ((header = (BANG_header*) extract_message(self,LENGTH_OF_HEADER)) != NULL) {
			switch (*header) {
				case BANG_HELLO:
					reading = peer_respond_hello(self);
					break;

				case BANG_DEBUG_MESSAGE:
					reading = read_debug_message(self);
					break;

				case BANG_SEND_MODULE:
					/* I guess we'll take it... */
					reading = read_module_message(self);
					break;

				case BANG_WANT_MODULE:
					/* TODO: Someone is asking us if we want a module... send out a signal! */
					break;

				case BANG_REQUEST_MODULE:
					/* TODO: This may be pretty hard to do. */
					break;

				case BANG_SEND_JOB:
					reading = read_job_message(self);

				case BANG_MISMATCH_VERSION:
				case BANG_BYE:
					reading = 0;
					break;
				default:
					/**
					 * Protocol mismatch, hang up.
					 */
#ifdef BDEBUG_1
					fprintf(stderr,"Protocol mismatch on peer read thread %d, hanging up.\n",self->peer_id);
#endif
					reading = 0;
					break;
			}

			free(header);
		} else {
			reading = 0;
		}
	}

#ifdef BDEBUG_1
	fprintf(stderr,"Peer read thread %d closing on self.\n",self->peer_id);
#endif

	read_peer_thread_self_close(self);
	return NULL;
}

void* BANG_write_peer_thread(void *self_info) {
	peer *self = (peer*)self_info;
	BANG_request *current;
	char sending = 1;
	BANG_header header;

	while (sending) {
		sem_wait(&(self->requests->num_requests));
		current = BANG_linked_list_dequeue(self->requests->requests);

		/*
		 * TODO: act on current request
		 */
		switch (current->type) {
			case BANG_CLOSE_REQUEST:
				header = BANG_BYE;
				write_message(self,&header,LENGTH_OF_HEADER);
				sending = 0;
				break;

			case BANG_SUDDEN_CLOSE_REQUEST:
				sending = 0;
				break;

			case BANG_DEBUG_REQUEST:
				/* TODO: ADD ERROR CHECKING!
				 * possibly put in own method */
				header = BANG_DEBUG_MESSAGE;
				write_message(self,&header,LENGTH_OF_HEADER);
				write_message(self,&(current->length),LENGTH_OF_LENGTHS);
				write_message(self,current->request,current->length);
				break;

			case BANG_SEND_MODULE_REQUEST:
				send_module(self,current->request);
				break;

			case BANG_MODULE_PEER_REQUEST:
				send_module_peer_request(self,current->request);
				break;

			default:
				/*ERROR!*/
#ifdef BDEBUG_1
				fprintf(stderr,"BANG ERROR:\t%d is not a request type that we take care of!\n",current->type);
#endif
				break;
		}

		free_BANG_request(current);
	}

#ifdef BDEBUG_1
	fprintf(stderr,"Write thread peer with peer_id %d is closing itself.\n",self->peer_id);
#endif
	return NULL;
}
