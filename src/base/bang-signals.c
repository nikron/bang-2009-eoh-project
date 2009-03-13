/**
 * \file bang-signals.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief  Implements the BANG signals.  Note: the signals will only be sent out one at a time, in
 * no particular order.
 */
#include"bang-signals.h"
#include"bang-utils.h"
#include"bang-types.h"
#include<errno.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/**
 * \brief The handlers for each of the signals is kept in a linked list stored
 *  in an array which is index by the signal's number
 */
BANG_linked_list *(signal_handlers[BANG_NUM_SIGS]);

void BANG_sig_init() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG sig starting.\n");
#endif

	int i;

	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		signal_handlers[i] = new_BANG_linked_list();
	}
}

void BANG_sig_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG sig closing.\n");
#endif
	int i;
	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		free_BANG_linked_list(signal_handlers[i],NULL);
	}
}

int BANG_install_sighandler(int signal, BANGSignalHandler handler) {
	/* We need to get a lock on the signal so that people aren't creating
	 * more that one signal at a time */

	BANG_linked_list_append(signal_handlers[signal],handler);
	return 0;
}

/**
 * Each signal must be sent in its own thread, so BANG_send_signal creates this structure
 * in order to pass arguments to a thread_send_signal pthread.
 */
typedef struct {
	/**
	 * Node of the handler located in signal linked list.
	 */
	BANGSignalHandler handler;
	/**
	 * The signal being sent to the handler.
	 */
	int signal;
	/**
	 * Arguements to the signal handler.
	 */
	void **handler_args;
	int num_handler_args;
} send_signal_args;

typedef struct {
	BANG_sigargs *args;
	int num_args;
	int signal;
} iterate_data;

/**
 * This is so that each signal can be sent in its own thread, and the signal caller
 * does not have to wait for handler to end.
 */
static void* threaded_send_signal(void *thread_args) {
	send_signal_args *h = (send_signal_args*)thread_args;
	h->handler(h->signal,h->num_handler_args,h->handler_args);
	free(h);
	return NULL;
}

static void send_signal(const void *c, void *d) {
	iterate_data *data = d;
	pthread_t signal_thread;
	int i;

	/* Used to transfer data to the thread. */
	send_signal_args *thread_args = malloc(sizeof(send_signal_args));

	thread_args->handler = c;
	thread_args->signal = data->signal;

	/* Each signal handler must have its own copy of the arguments. */
	thread_args->handler_args = calloc(data->num_args,sizeof(void*));

	for (i = 0; i < data->num_args; ++i) {
		thread_args->handler_args[i] = malloc(data->args[i].length);
		memcpy(thread_args->handler_args[i],data->args[i].args,data->args[i].length);
	}

	thread_args->num_handler_args = data->num_args;


	pthread_create(&signal_thread,NULL,threaded_send_signal,(void*)thread_args);

	/* We wont try to keep track of these threads */
	pthread_detach(signal_thread);
}

int BANG_send_signal(int signal, BANG_sigargs *args, int num_args) {

#ifdef BDEBUG_1
	fprintf(stderr,"Sending out the signal %d.\n",signal);
	fprintf(stderr,"The signal_node is %p.\n",signal_handlers[signal]);
#endif

	iterate_data d = { args, num_args, signal };
	BANG_linked_list_iterate(signal_handlers[signal],&send_signal,&d);

	return 0;
}
