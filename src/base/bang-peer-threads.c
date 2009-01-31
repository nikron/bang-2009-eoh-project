#include"bang-peer.h"
#include"bang-peer-threads.h"
#include"bang-routing.h"
#include"bang-signals.h"
#include"bang-types.h"
#include<poll.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<uuid/uuid.h>

/**
 * \param self The peer to be closed.
 *
 * \brief A peer thread asks to close itself.
 */
static void read_peer_thread_self_close(BANG_peer *self);

/**
 * \param self The peer wanting to extract a message from the socket.
 * \param length The length of the message to extract.
 *
 * \brief Extracts a message of length length.
 */
static void* extract_message(BANG_peer *self, unsigned int length);

static unsigned int write_message(BANG_peer *self, void *message, unsigned int length);

/**
 * \param self The peer responding to BANG_HELLO.
 *
 * \brief Acts on an incoming BANG_HELLO.
 */
static char peer_respond_hello(BANG_peer *self);
/*
 * HERE THERE BE DRAGONS!
 *
 * Or, actual peer logic starts here...
 */

static char peer_respond_hello(BANG_peer *self) {
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

static char read_debug_message(BANG_peer *self) {
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

static char read_module_message(BANG_peer *self) {
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

static void extract_uuid(BANG_peer *self,uuid_t uuid) {
	uuid_t *uuid_ptr = (uuid_t*) extract_message(self,sizeof(uuid_t));
	if (uuid_ptr) {
		uuid_copy(uuid,*uuid_ptr);
		free(uuid_ptr);
	} else {
		uuid_clear(uuid);
	}
}

static char read_job_message(BANG_peer *self) {
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
static void send_module(BANG_peer *self, BANG_request *request) {
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

static void send_module_peer_request(BANG_peer *self, BANG_request *request) {
	BANG_header header = BANG_WANT_MODULE;
	write_message(self,&header,LENGTH_OF_HEADER);
	write_message(self,request->request,request->length);
}

static void read_peer_thread_self_close(BANG_peer *self) {
	BANG_sigargs args;
	args.args = &(self->peer_id);
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_DISCONNECTED,&args,1);

	pthread_exit(NULL);
}

static unsigned int write_message(BANG_peer *self, void *message, unsigned int length) {
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

static void* extract_message(BANG_peer *self, unsigned int length) {
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
	BANG_peer *self = (BANG_peer*)self_info;

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
	BANG_peer *self = (BANG_peer*)self_info;
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
