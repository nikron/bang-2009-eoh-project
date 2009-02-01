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

static unsigned int write_message(BANG_peer *self, void *message, unsigned int length);

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
static void write_module(BANG_peer *self, BANG_request *request);

static void write_module_exists(BANG_peer *self, BANG_request *request);

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

static void write_module(BANG_peer *self, BANG_request *request) {
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

static void write_module_exists(BANG_peer *self, BANG_request *request) {
	BANG_header header = BANG_EXISTS_MODULE;
	write_message(self,&header,LENGTH_OF_HEADER);
	write_message(self,request->request,request->length);
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
				write_module(self,current->request);
				break;

			case BANG_MODULE_PEER_REQUEST:
				write_module_exists(self,current->request);
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
