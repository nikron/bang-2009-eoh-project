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
 * 
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
static void* read_message(BANG_peer *self, unsigned int length);

static void read_uuid(BANG_peer *self,uuid_t uuid);

/**
 * \param self The peer responding to BANG_HELLO.
 *
 * \brief Acts on an incoming BANG_HELLO.
 */
static char read_hello(BANG_peer *self);

static char read_debug_message(BANG_peer *self);

static char read_module_message(BANG_peer *self);

static char read_job_message(BANG_peer *self);

static void read_peer_thread_self_close(BANG_peer *self) {
	BANG_sigargs args;
	args.args = &(self->peer_id);
	args.length = sizeof(int);

	BANG_send_signal(BANG_PEER_DISCONNECTED,&args,1);

	pthread_exit(NULL);
}

static void* read_message(BANG_peer *self, unsigned int length) {
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
static void read_uuid(BANG_peer *self,uuid_t uuid) {
	uuid_t *uuid_ptr = (uuid_t*) read_message(self,sizeof(uuid_t));
	if (uuid_ptr) {
		uuid_copy(uuid,*uuid_ptr);
		free(uuid_ptr);
	} else {
		uuid_clear(uuid);
	}
}

static char read_hello(BANG_peer *self) {
	unsigned char *version = (unsigned char*) read_message(self,LENGTH_OF_VERSION);
	/* TODO: Make this more elegant =P */
	if (BANG_version_cmp(version,BANG_LIBRARY_VERSION) == 0) {
		free(version);
		return 0;
	}
	free(version);

	unsigned int *length = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}
	self->peername = (char*) read_message(self,*length);
	free(length);
	return 1;
}

static char read_debug_message(BANG_peer *self) {
	unsigned int *length = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}
	char *message = (char*) read_message(self,*length);
	if (message == NULL) {
		return 0;
	}
	fprintf(stderr,"%s",message);
	free(message);
	return 1;

}

static char read_module_message(BANG_peer *self) {
	unsigned int *length = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (length == NULL) {
		return 0;
	}

	BANG_sigargs args;
	args.args = (char*) read_message(self,*length);

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

static char read_job_message(BANG_peer *self) {
	uuid_t auth, peer;

	read_uuid(self,auth);
	if (uuid_is_null(auth)) {
		return 0;
	}

	read_uuid(self,peer);
	if (uuid_is_null(peer)) {
		return 0;
	}

	BANG_job job;

	int *job_number = (int*) read_message(self,SIZE_JOB_NUMBER);
	if (job_number == NULL) {
		return 0;
	}

	job.job_number = *job_number;
	free(job_number);

	unsigned int *job_length  = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (job_length == NULL) {
		return 0;
	}

	job.length = *job_length;
	free(job_length);

	job.data = read_message(self,job.length);
	if (job.data == NULL) {
		return 0;
	}

	BANG_route_job(auth,peer,&job);

	return 1;
}

static char read_module_exists(BANG_peer *self) {
	
	unsigned int *mod_name_length;
	if ((mod_name_length = (unsigned int*) read_message(self,4)) == NULL)
		return 0;
	
	char *mod_args;
	if ((mod_args = (char*) read_message(self, (*mod_name_length+4))) == NULL)
		return 0;
	
	// create new buffer to hold peer_id plus module name and version
	char *mod_buffer = (char*)malloc(*mod_name_length+4+sizeof(int));
	
	*mod_buffer = *mod_args;
	
	// Set last sizeof(int) bits to peer_id - may not be a good way to do this.
	mod_buffer[*mod_name_length+4] = self->peer_id;
	
	BANG_sigargs mod_exists_args;
	
	mod_exists_args.args = (void*) mod_buffer;
	mod_exists_args.length = *mod_name_length+4+sizeof(int);
	
	free(mod_name_length);
	
	BANG_send_signal(BANG_MODULE_EXISTS,&mod_exists_args,1);
	free(mod_buffer);
	free(mod_args);
	
	return 1;
}

static char read_module_request(BANG_peer *self) {
		
	unsigned int *mod_name_length;
	if ((mod_name_length = (unsigned int*) read_message(self,4)) == NULL)
		return 0;
	
	char *mod_args;
	if ((mod_args = (char*) read_message(self, (*mod_name_length+4))) == NULL)
		return 0;
	
	// create new buffer to hold peer_id plus module name and version
	char *mod_buffer = (char*)malloc(*mod_name_length+4+sizeof(int));
	
	*mod_buffer = *mod_args;
	
	// Set last sizeof(int) bits to peer_id - may not be a good way to do this.
	mod_buffer[*mod_name_length+4] = self->peer_id;
	
	BANG_sigargs mod_exists_args;
	
	mod_exists_args.args = (void*) mod_buffer;
	mod_exists_args.length = *mod_name_length+4+sizeof(int);
	
	free(mod_name_length);
	
	BANG_send_signal(BANG_MODULE_REQUEST,&mod_exists_args,1);
	free(mod_buffer);
	free(mod_args);
	
	return 1;
}

static char read_request_job(BANG_peer *self) {
	uuid_t auth, peer;

	read_uuid(self,auth);
	if (uuid_is_null(auth)) {
		return 0;
	}

	read_uuid(self,peer);
	if (uuid_is_null(peer)) {
		return 0;
	}
	
	BANG_route_request_job(peer, auth);
	return 1;
}

static char read_available_job(BANG_peer *self) {
	uuid_t auth, peer;

	read_uuid(self,auth);
	if (uuid_is_null(auth)) {
		return 0;
	}

	read_uuid(self,peer);
	if (uuid_is_null(peer)) {
		return 0;
	}
	
	BANG_job job;
	
	int *job_number = (int*) read_message(self,SIZE_JOB_NUMBER);
	if (job_number == NULL) {
		return 0;
	}

	job.job_number = *job_number;
	free(job_number);

	unsigned int *job_length  = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (job_length == NULL) {
		return 0;
	}

	job.length = *job_length;
	free(job_length);

	job.data = read_message(self,job.length);
	if (job.data == NULL) {
		return 0;
	}
	
	BANG_route_finished_job(auth, peer, &job);
	return 1;
}

static char read_finished_job(BANG_peer *self) {
	uuid_t auth, peer;

	read_uuid(self,auth);
	if (uuid_is_null(auth)) {
		return 0;
	}

	read_uuid(self,peer);
	if (uuid_is_null(peer)) {
		return 0;
	}
	
	BANG_job job;
	
	/* MAGIC NUMBER AGAIN */
	int *job_number = (int*) read_message(self,4);
	if (job_number == NULL) {
		return 0;
	}

	job.job_number = *job_number;
	free(job_number);

	unsigned int *job_length  = (unsigned int*) read_message(self,LENGTH_OF_LENGTHS);
	if (job_length == NULL) {
		return 0;
	}

	job.length = *job_length;
	free(job_length);

	job.data = read_message(self,job.length);
	if (job.data == NULL) {
		return 0;
	}
	
	BANG_route_finished_job(auth, peer, &job);
	
	return 1;
}

void* BANG_read_peer_thread(void *self_info) {
	BANG_peer *self = (BANG_peer*)self_info;

	BANG_header *header;

	char reading = 1;

	while (reading) {
		if ((header = (BANG_header*) read_message(self,LENGTH_OF_HEADER)) != NULL) {
			switch (*header) {
				case BANG_HELLO:
					reading = read_hello(self);
					break;

				case BANG_DEBUG_MESSAGE:
					reading = read_debug_message(self);
					break;

				case BANG_SEND_MODULE:
					/* I guess we'll take it... */
					reading = read_module_message(self);
					break;

				case BANG_EXISTS_MODULE:
					/* Someone is asking us if we want a module... send out a signal! */
					reading = read_module_exists(self);
					break;

				case BANG_REQUEST_MODULE:
					/* Send signal to request the new module. */
					reading = read_module_request(self);
					break;

				case BANG_SEND_JOB:
					reading = read_job_message(self);
					break;
				
				case BANG_REQUEST_JOB:
					reading = read_request_job(self);
					break;
				
				case BANG_FINISHED_JOB:
					reading = read_finished_job(self);
					break;
				
				case BANG_AVAILABLE_JOB:
					reading = read_available_job(self);
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
