/**
 * \file bang-signals.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief  Implements the BANG signals.  Note: the signals will only be sent out one at a time, in
 * no particular order.
 *
 */
#include"bang-signals.h"
#include"bang-types.h"
#include<errno.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

///The handlers for each signal is stored in linked list.
struct _signal_node {
	BANGSignalHandler handler;
	struct _signal_node *next;
};

typedef struct _signal_node signal_node;

/**
 * \param head The head of the signal_node list.
 *
 * \brief Frees a signal node linked list.
 */
void recursive_sig_free(signal_node *head) {
	if (head == NULL) return;
	if (head->next != NULL) {
		recursive_sig_free(head->next);
	}
	free(head);
	head = NULL;
}

/**
 * \brief The handlers for each of the signals is kept in a linked list stored
 *  in an array which is index by the signal's number
 */
signal_node **signal_handlers;

void BANG_sig_init() {
	int i;

	signal_handlers = (signal_node**) calloc(BANG_NUM_SIGS,sizeof(signal_node*));
	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		signal_handlers[i] = NULL;
	}
}

void BANG_sig_close() {
	fprintf(stderr,"BANG sig closing.\n");
	int i;
	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		recursive_sig_free(signal_handlers[i]);
	}
	free(signal_handlers);
	signal_handlers = NULL;
}

int BANG_install_sighandler(int signal, BANGSignalHandler handler) {
	if (signal_handlers[signal] == NULL) {
		signal_handlers[signal] = (signal_node*) malloc(sizeof(signal_node));
		signal_handlers[signal]->handler = handler;
		signal_handlers[signal]->next = NULL;
		return 0;
	} else {
		signal_node *cur;
		for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
			if (cur->next == NULL) {
				cur->next =(signal_node*) malloc(sizeof(signal_node));
				cur->next->handler = handler;
				cur->next->next = NULL;
				return 0;
			}
		}
	}
	return -1;
}

///Each signal must be sent in its own thread, so BANG_send_signal creates this structure
///in order to pass arguements to a thread_send_signal pthread
typedef struct {
	///Node of the handler located in signal linked list.
	signal_node *signode;
	///The signal being sent to the handler.
	int signal;
	int sig_id;
	///Arguements to the signal handler.
	void *handler_args;
} send_signal_args;

///This is so that each signal can be sent in its own thread, and the signal caller
///does not have to wait for handler to end.
void* threaded_send_signal(void *thread_args) {
	send_signal_args *handler = (send_signal_args*) thread_args;
	handler->signode->handler(handler->signal,handler->sig_id,handler->handler_args);
	free(handler);
	handler = NULL;
	return NULL;
}

int BANG_send_signal(int signal, BANG_sigargs args) {
#ifdef BDEBUG_1
	fprintf(stderr,"Sending out the signal %d.\n",signal);
	fprintf(stderr,"The signal_node is %p.\n",signal_handlers[signal]);
#endif

	signal_node *cur;
	send_signal_args *thread_args;
	pthread_t signal_thread;

	int i = 0;
	for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
		thread_args = (send_signal_args*) calloc(1,sizeof(send_signal_args));

		if (args.args == NULL) {
			thread_args->handler_args = NULL;
		} else {
			thread_args->handler_args = calloc(args.length,1);
			memcpy(thread_args->handler_args,args.args,args.length);
		}

		thread_args->signode = cur;
		thread_args->signal = signal;
		thread_args->sig_id = (signal << (sizeof(int) * 8 / 2));

		pthread_create(&signal_thread,NULL,threaded_send_signal,(void*)thread_args);
		pthread_detach(signal_thread);

		++i;
	}
	return 0;
}
