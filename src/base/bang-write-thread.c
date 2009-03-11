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
static char write_module(BANG_peer *self, BANG_request *request);

/*
 * \param self The peer sending the module.
 *
 * \brief Writes a module out to the remote end.
 *
 * \return
 * 0: Length of data sent != length of data requested for sending
 * 1: Data was sent
*/
static char write_module_exists(BANG_peer *self, BANG_request *request);

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

/*
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
static char send_header_and_request(BANG_peer *self, BANG_header header,
							BANG_request *request) {
	//If the amount sent is not the amount we wanted to send, return 0
	return !((write_message(self,&header,LENGTH_OF_HEADER) != LENGTH_OF_HEADER) ||
	    (write_message(self,request->request,request->length) != request->length));

}

//----------------------Actions performed on requests--------------------------.
static char write_bye(BANG_peer *self) {
	BANG_header header = BANG_BYE;

	write_message(self,&header,LENGTH_OF_HEADER);

	return 0;
}

/*
BANG_DEBUG_MESSAGE->
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
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
	return send_header_and_request(self, BANG_EXISTS_MODULE, request);
}

/*
BANG_SEND_JOB->BANG_SEND_JOB_REQUEST
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
static char write_send_job(BANG_peer *self, BANG_request *request) {
	return send_header_and_request(self, BANG_SEND_JOB, request);
}

/*
BANG_FINISHED_JOB->BANG_SEND_FINISHED_JOB_REQUEST
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
static char write_finished_job(BANG_peer *self, BANG_request *request) {
	return send_header_and_request(self, BANG_FINISHED_JOB, request);
}

/*
BANG_REQUEST_JOB->BANG_SEND_REQUEST_JOB_REQUEST
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
static char write_request_job(BANG_peer *self, BANG_request *request) {
	return send_header_and_request(self, BANG_REQUEST_JOB, request);
}

/*
BANG_AVAILABLE_JOB->BANG_SEND_AVAILABLE_JOB_REQUEST
Returns:
	0: Length of data sent != length of data requested for sending
	1: Data was sent 
*/
static char write_available_job(BANG_peer *self, BANG_request *request) {
	return send_header_and_request(self, BANG_AVAILABLE_JOB, request);
}

static char write_module(BANG_peer *self, BANG_request *request) {
	FILE *fd = fopen((char*)request->request,"r");
	if (fd == NULL) {
		return 0;
	}

	char chunk[UPDATE_SIZE], ret;
	size_t reading;
	unsigned int i;

	i = BANG_SEND_MODULE;
	ret = write_message(self,&i,LENGTH_OF_HEADER);

	/* TODO: Error checking */
	fseek(fd,0,SEEK_END);
	i = (unsigned int) ftell(fd);
	rewind(fd);

	ret = write_message(self,&i,LENGTH_OF_LENGTHS);
	if (ret == 0) return ret;

	do {
		/* TODO: Error checking */
		reading = fread(chunk,UPDATE_SIZE,1,fd);
		ret = write_message(self,chunk,reading);
		if (ret == 0) return ret;

	} while (reading >= UPDATE_SIZE);

	fclose(fd);

	return 1;
}
/* ----------------------Actions performed on requests-------------------------- */


//-------------------------Request processing----------------------------------.
void* BANG_write_peer_thread(void *self_info) {
	BANG_peer *self = (BANG_peer*)self_info;
	BANG_request *current;
	char sending = 1;

	while (sending) {
		sem_wait(&(self->requests->num_requests));
		current = BANG_linked_list_dequeue(self->requests->requests);

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
			case BANG_SEND_JOB_REQUEST:
				sending = write_send_job(self,current->request);
			break;
			case BANG_SEND_FINISHED_JOB_REQUEST:
				sending = write_finished_job(self,current->request);
			break;
			case BANG_SEND_REQUEST_JOB_REQUEST:
				sending = write_request_job(self,current->request);
			break;
			case BANG_SEND_AVAILABLE_JOB_REQUEST:
				sending = write_available_job(self,current->request);
			break;
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
