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

//Does the actual writing
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

//----------------------Actions performed on requests--------------------------.
static char write_bye(BANG_peer *self) {
	BANG_header header = BANG_BYE;

	write_message(self,&header,LENGTH_OF_HEADER);

	return 0;
}

//BANG_DEBUG_MESSAGE->
static char write_debug(BANG_peer *self, BANG_request *request) {
	BANG_header header = BANG_DEBUG_MESSAGE;

	unsigned int ret = write_message(self,&header,LENGTH_OF_HEADER);

	if (ret < LENGTH_OF_HEADER)
		return 0;

	ret = write_message(self,&(request->length),LENGTH_OF_LENGTHS);

	if (ret < LENGTH_OF_LENGTHS)
		return 0;

	write_message(self,request->request,request->length);

	if (ret < request->length)
		return 0;
	else
		return 1;

}

static char write_module_exists(BANG_peer *self, BANG_request *request) {
	BANG_header header = BANG_EXISTS_MODULE;
	write_message(self,&header,LENGTH_OF_HEADER);
	write_message(self,request->request,request->length);
}





//-----------------------------------------------------------------------------.
//BANG_SEND_JOB->BANG_SEND_JOB_REQUEST
static char write_send_job(BANG_peer *self, BANG_request *request) {
	 * message:
	 *	-BANG_SEND_JOB		(4 unsigned bytes)
	 *	-Authority		(16 bytes)
	 *	-Peer			(16 bytes)
	 *	-job_numer		(4 bytes)
	 *	-job_length		(4 unsigned bytes)
	 *	-job_data		(job_length bytes)
	BANG_header header = BANG_SEND_JOB;
	write_message(self,&header,LENGTH_OF_HEADER);
	write_message(self,request->request,request->length);
}


///TODO Change header "BANG_WRITE_FINISHED_JOB" to "BANG_FINISHED_JOB" TODO
//BANG_FINISHED_JOB->BANG_SEND_FINISHED_JOB_REQUEST
static char write_finished_job() {
	BANG_header header = BANG_FINISHED_JOB;
	write_message(self,&header,LENGTH_OF_HEADER);
}

//BANG_REQUEST_JOB->BANG_SEND_REQUEST_JOB_REQUEST
static char write_request_job() {
	BANG_header header = BANG_REQUEST_JOB;
	write_message(self,&header,LENGTH_OF_HEADER);

}

///TODO Add header "BANG_AVAILABLE_JOB" to BANG_SEND_AVAILABLE_JOB_REQUEST TODO
//BANG_AVAILABLE_JOB->BANG_SEND_AVAILABLE_JOB_REQUEST
static char write_available_job() {
	BANG_header header = BANG_AVAILABLE_JOB; 
	write_message(self,&header,LENGTH_OF_HEADER);

}
//-----------------------------------------------------------------------------'





static char write_module(BANG_peer *self, BANG_request *request) {
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
//----------------------Actions performed on requests--------------------------'


//-------------------------Request processing----------------------------------.
void* BANG_write_peer_thread(void *self_info) {
	BANG_peer *self = (BANG_peer*)self_info;
	BANG_request *current;
	char sending = 1;

	while (sending) {
		sem_wait(&(self->requests->num_requests));
		current = BANG_linked_list_dequeue(self->requests->requests);

		/*
		 * TODO: act on current request
		 */
		switch (current->type) {
			case BANG_CLOSE_REQUEST:
				sending = write_bye(self);
			break;

			case BANG_SUDDEN_CLOSE_REQUEST:
				sending = 0;
			break;

			case BANG_DEBUG_REQUEST:
				sending = write_debug(self,current);
			break;

			case BANG_MODULE_PEER_REQUEST:
				sending = write_module_exists(self,current->request);
			break;
//-----------------------------------------------------------------------------.
			case BANG_SEND_JOB_REQUEST:
				sending = write_send_job();
			break;

			case BANG_SEND_FINISHED_JOB_REQUEST:
				sending = write_finished_job();
			break;

			case BANG_SEND_REQUEST_JOB_REQUEST:
				sending = write_request_job();
			break;

			case BANG_SEND_AVAILABLE_JOB_REQUEST:
				sending = write_available_job();
			break;
//-----------------------------------------------------------------------------'
			case BANG_SEND_MODULE_REQUEST:
				sending = write_module(self,current->request);
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
//-------------------------Request processing----------------------------------'
