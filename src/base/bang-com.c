/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Implements "the master of slave peer threads" model.
 */
#include"bang-com.h"
#include"bang-net.h"
#include"bang-signals.h"
#include"bang-utils.h"
#include"bang-types.h"
#include<poll.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>

/**
 * A linked list of requests of peer send threads.
 */
typedef struct _request_node {
	BANG_request request;
	/**
	 * The next node in the request list.
	 */
	struct _request_node *next;
} request_node;

/**
 * Holds requests of the peers in a linked list.
 */
typedef struct {
	/**
	 * The lock on adding or removing requests.
	 */
	pthread_mutex_t lock;
	/**
	 * Signals the thread that a new requests has been added.
	 */
	sem_t num_requests;
	/**
	 * A linked list of requests
	 */
	request_node *head;
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
	/**
	 * Modules that this peer is aware that are running.
	 * memory: char* is not the responsibility of the peer.
	 */
	pthread_mutex_t my_modules_lock;
	char **my_modules;
	unsigned int my_modules_len;
	/**
	 * Modules that the remote is running.
	 */
	pthread_mutex_t remote_modules_lock;
	char **remote_modules;
	unsigned int remote_modules_len;
} peer;

/**
 * \return The head of the request node list starting at head.
 *
 * \brief Pops off the head of the linked list.
 */
static request_node* pop_request(request_node **head);

/**
 * \param node appends node to list started at head
 * \param head start of the request node list
 *
 * \brief Appends a node to the head linked list.
 */
static void append_request(request_node **head, request_node *node);

/**
 * \param head The head of the list to be freed.
 *
 * \brief Frees resources used by a request list started at
 * head.
 */
static void free_request_list(request_node *head);

/**
 * \brief Allocates and returns a new request node.
 */
static request_node* new_request_node();

/**
 * \param self The peer to be requested.
 * \param request Request to be given to the peer.
 *
 * \brief Adds a request to a peer structure.
 */
static void request_peer(peer *self, BANG_request request);

/**
 * \brief Removes and frees a peer and its resources.
 */
static void free_peer(peer *p);

/**
 * \brief Allocates and returns a new peer.
 */
static peer* new_peer();

/*
 * \param requests The requests to be freed.
 *
 * \brief Frees a BANGRequests struct.
 */
static void free_BANG_requests(BANG_requests *requests);

/*
 * \return Returns an initialized BANGRequest pointer.
 */
static BANG_requests* new_BANG_requests();

/*
 * \param self The peer to be closed.
 *
 * \brief A peer thread asks to close itself.
 */
static void read_peer_thread_self_close(peer *self);

/**
 * This is a lock on readers
 */
static pthread_mutex_t peers_read_lock;

/**
 * The number of threads currently reading for the peers structure
 */
static int peers_readers = 0;

/**
 * A lock on changing the size of peers
 */
static pthread_mutex_t peers_change_lock;

/**
 * The total number of peers we get through the length of the program.
 */
static unsigned int peer_count = 0;

/**
 * The current number of peers connected to the program.
 */
static unsigned int current_peers = 0;

/**
 * Information and threads for each peer connected to the program
 */
static peer **peers = NULL;

/**
 * TODO: make this structure not take linear time when sending a request
 * to a specific peer
 */
static int *keys = NULL;

static void acquire_peers_read_lock() {
	BANG_acquire_read_lock(&peers_readers,&peers_read_lock,&peers_change_lock);
}

static void release_peers_read_lock() {
	BANG_release_read_lock(&peers_readers,&peers_read_lock,&peers_change_lock);
}

static void free_peer(peer *p) {
#ifdef BDEBUG_1
	fprintf(stderr,"Freeing a peer with peer_id %d.\n",p->peer_id);
#endif
	int i;
	pthread_cancel(p->receive_thread);

	/* We need to close the receive thread in a more roundabout way, since it may be waiting
	 * on a semaphore in which case it will never close */
	BANG_request request;

	request.type = BANG_CLOSE_REQUEST;
	request.length = 0;
	request.request = NULL;

	request_peer(p,request);

	pthread_join(p->receive_thread,NULL);
	pthread_join(p->send_thread,NULL);

	free_BANG_requests(p->requests);

	if (p->remote_modules) {
		for (i = 0; p->remote_modules[i]; ++i) {
			free(p->remote_modules[i]);
		}
		free(p->my_modules);
	}


	close(p->socket);

	pthread_mutex_unlock(&(p->my_modules_lock));
	pthread_mutex_unlock(&(p->remote_modules_lock));
	pthread_mutex_destroy(&(p->my_modules_lock));
	pthread_mutex_destroy(&(p->remote_modules_lock));
	free(p);
}

static request_node* pop_request(request_node  **head) {
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

static void append_request(request_node **head, request_node *node) {
	if (*head == NULL) {
		*head = node;
	} else {
		request_node *cur;
		for (cur = *head; cur->next != NULL; cur = cur->next);
		cur->next = node;
	}
}

static void free_request_list(request_node *head) {
	if (head == NULL) return;
	if (head->next != NULL)
		free_request_list(head->next);
	free(head);
}

static request_node* new_request_node() {
	request_node *new_node = (request_node*) calloc(1,sizeof(request_node));
	new_node->next = NULL;
	return new_node;
}

static BANG_requests* new_BANG_requests() {
	BANG_requests *requests = (BANG_requests*) calloc(1,sizeof(BANG_requests));
	requests->head = NULL;
	pthread_mutex_init(&(requests->lock),NULL);
	sem_init(&(requests->num_requests),0,0);
	return requests;
}

static void free_BANG_requests(BANG_requests *requests) {
	pthread_mutex_destroy(&(requests->lock));
	sem_destroy(&(requests->num_requests));
	free_request_list(requests->head);
}

static void read_peer_thread_self_close(peer *self) {
	BANG_sigargs args;
	args.args = calloc(1,sizeof(int));
	*((int*)args.args) = self->peer_id;
	args.length = sizeof(int);

	acquire_peers_read_lock();
	BANG_send_signal(BANG_PEER_DISCONNECTED,&args,1);
	free(args.args);
	release_peers_read_lock();

	pthread_exit(NULL);
}

/**
 *
 * \param self The peer wanting to extract a message from the socket.
 * \param length The length of the message to extract.
 *
 * \brief Extracts a message of length length.
 */
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

static char peer_respond_hello(peer *self) {
	unsigned char *version = (unsigned char*) extract_message(self,LENGTH_OF_VERSION);
	/* TODO: Make this more elegant =P */
	if (version == NULL || version[0] != BANG_MAJOR_VERSION || version[1] != BANG_MIDDLE_VERSION || version[2] != BANG_MINOR_VERSION) {
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

void* BANG_read_peer_thread(void *self_info) {
	peer *self = (peer*)self_info;
	memset(&(self->pfd),0,sizeof(struct pollfd));
	self->pfd.fd = self->socket;
	self->pfd.events = POLLIN  | POLLERR | POLLHUP | POLLNVAL;

	unsigned int *header;

	char reading = 1;

	while (reading) {
		if ((header = (unsigned int*) extract_message(self,LENGTH_OF_HEADER)) != NULL) {
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
static void send_module(peer *self, BANG_request request) {
	FILE *fd = fopen((char*)request.request,"r");
	if (fd == NULL) {
		free(request.request);
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
	free(request.request);
}

static void send_module_peer_request(peer *self, BANG_request request) {
	unsigned int header = BANG_WANT_MODULE;
	write_message(self,&header,LENGTH_OF_LENGTHS);
	write_message(self,&(request.request),request.length);
	free(request.request);
}

static void register_module_name(peer *self, char *my_module_name) {
	/* TODO: READER LOCKS? */
	pthread_mutex_lock(&(self->my_modules_lock));
	self->my_modules = realloc(self->my_modules, self->my_modules_len++ * sizeof(char*) + 1);
	self->my_modules[self->my_modules_len - 1] = my_module_name;
	self->my_modules[self->my_modules_len] = NULL;

	/* Should we send something to the remote end? */
}

void* BANG_write_peer_thread(void *self_info) {
	peer *self = (peer*)self_info;
	request_node *current;
	char sending = 1;
	unsigned int header;
	while (sending) {
		sem_wait(&(self->requests->num_requests));
		pthread_mutex_lock(&(self->requests->lock));
		current = pop_request(&(self->requests->head));
		pthread_mutex_unlock(&(self->requests->lock));

		/*
		 * TODO: act on current request
		 */
		switch (current->request.type) {
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
				write_message(self,&(current->request.length),LENGTH_OF_LENGTHS);
				write_message(self,&(current->request.request),current->request.length);
				free(current->request.request);
				break;

			case BANG_SEND_MODULE_REQUEST:
				send_module(self,current->request);
				break;

			case BANG_MODULE_PEER_REQUEST:
				send_module_peer_request(self,current->request);
				break;

			case BANG_MODULE_REGISTER_REQUEST:
				register_module_name(self,(char*)current->request.request);
				free(current->request.request);
				break;

			default:
				/*ERROR!*/
#ifdef BDEBUG_1
				fprintf(stderr,"BANG ERROR:\t%d is not a request type that we take care of!\n",current->request.type);
#endif
				break;
		}

		free(current);
	}

#ifdef BDEBUG_1
	fprintf(stderr,"Write thread peer with peer_id %d is closing itself.\n",self->peer_id);
#endif
	return NULL;
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

static void request_peer(peer *to_be_requested, BANG_request request) {
	pthread_mutex_lock(&(to_be_requested->requests->lock));

	request_node *temp = new_request_node();
	temp->request = request;
	append_request(&(to_be_requested->requests->head),temp);

	pthread_mutex_unlock(&(to_be_requested->requests->lock));
	sem_post(&(to_be_requested->requests->num_requests));
}

void BANG_request_all(BANG_request request) {
	unsigned int i = 0;

	acquire_peers_read_lock();

	for (; i < current_peers; ++i) {
		request_peer(peers[i],request);
	}

	release_peers_read_lock();
}

static void catch_request_all(int signal, int num_requests, void **vrequest) {
	if (signal == BANG_REQUEST_ALL) {
		BANG_request **to_request  = (BANG_request**) vrequest;
		int i = 0;
		for (; i < num_requests; ++i) {
			BANG_request_all(*(to_request[i]));
			free(to_request[i]);
		}
		free(to_request);
	}
}

void BANG_request_peer_id(int peer_id, BANG_request request) {
	int id;
	acquire_peers_read_lock();
	id = BANG_get_key_with_peer_id(peer_id);
	request_peer(peers[id],request);
	release_peers_read_lock();
}

int BANG_get_key_with_peer_id(int peer_id) {
	unsigned int i = 0;
	for (i = 0; i < current_peers; ++i) {
		if (peers[i]->peer_id == peer_id) {
			return i;
		}
	}

	return -1;
}

static peer* new_peer() {
	peer *new;
	new = (peer*) calloc(1,sizeof(peer));
	new->requests = new_BANG_requests();
	new->my_modules_len = 0;
	new->remote_modules_len = 0;
	pthread_mutex_init(&(new->remote_modules_lock),NULL);
	pthread_mutex_init(&(new->my_modules_lock),NULL);
	return new;
}

void BANG_add_peer(int socket) {
	pthread_mutex_lock(&peers_change_lock);

	++current_peers;
	int current_key = current_peers - 1;
	int current_id = peer_count++;

	keys = (int*) realloc(keys,current_peers * sizeof(int));
	keys[current_key] = current_id;

	peers = (peer**) realloc(peers,current_peers * sizeof(peer*));
	peers[current_key] = new_peer();
	peers[current_key]->peer_id = current_id;
	peers[current_key]->socket = socket;

#ifdef BDEBUG_1
	fprintf(stderr,"Threads being started at %d.\n",current_key);
#endif
	pthread_create(&(peers[current_key]->receive_thread),NULL,BANG_read_peer_thread,peers[current_key]);
	pthread_create(&(peers[current_key]->send_thread),NULL,BANG_write_peer_thread,peers[current_key]);

	pthread_mutex_unlock(&peers_change_lock);

	/**
	 * Send out that we successfully started the peer threads.
	 */
	BANG_sigargs args;
	args.args = calloc(1,sizeof(int));
	*((int*)args.args) = current_id;
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_ADDED,&args,1);

	free(args.args);
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

void BANG_remove_peer(int peer_id) {
	/*
	 * this lock is needed when trying to change the
	 * peers structure
	 */
#ifdef BDEBUG_1
	fprintf(stderr,"Removing peer %d.\n",peer_id);
#endif

	pthread_mutex_lock(&peers_change_lock);

	int pos = BANG_get_key_with_peer_id(peer_id);
	if (pos == -1) return;

	free_peer(peers[pos]);
	peers[pos] = NULL;

	for (;((unsigned int)pos) < current_peers - 1; ++pos) {
		peers[pos] = peers[pos + 1];
		keys[pos] = keys[pos + 1];
	}

	--current_peers;


	peers = (peer**) realloc(peers,current_peers * sizeof(peer*));
	keys = (int*) realloc(keys,current_peers * sizeof(int));

	pthread_mutex_unlock(&peers_change_lock);

	BANG_sigargs peer_send;
	peer_send.args = calloc(1,sizeof(int));
	*((int*)peer_send.args) = peer_id;
	peer_send.length = sizeof(int);
	BANG_send_signal(BANG_PEER_REMOVED,&peer_send,1);
	free(peer_send.args);
}

void BANG_com_init() {
	BANG_install_sighandler(BANG_PEER_CONNECTED,&catch_add_peer);
	BANG_install_sighandler(BANG_PEER_DISCONNECTED,&catch_remove_peer);
	BANG_install_sighandler(BANG_REQUEST_ALL,&catch_request_all);
	pthread_mutex_init(&peers_change_lock,NULL);
	pthread_mutex_init(&peers_read_lock,NULL);
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
	unsigned int i = 0;
	pthread_mutex_lock(&peers_change_lock);
	for (i = 0; i < current_peers; ++i) {
		free_peer(peers[i]);
	}

	pthread_mutex_unlock(&peers_change_lock);

	pthread_mutex_destroy(&peers_change_lock);
	pthread_mutex_destroy(&peers_read_lock);
	free(peers);
	free(keys);
	keys = NULL;
	peers = NULL;
	current_peers = 0;
	peer_count = 0;
}
