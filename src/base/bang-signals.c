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

void recursive_sig_free(signal_node *head) {
	if (head->next != NULL) {
		recursive_sig_free(head->next);
	}
	free(head);
	head = NULL;
}

void BANG_sig_close() {
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

typedef struct {
	signal_node *signode;
	int signal;
	int sig_id;
	void *handler_args;
} send_signal_args;

int BANG_send_signal(int signal, BANG_sigargs args) {
#ifdef BDEBUG_1
	fprintf(stderr,"Sending out the signal %d.\n",signal);
	fprintf(stderr,"The signal_node is %p.\n",signal_handlers[signal]);
#endif

	signal_node *cur;

	int i = 0;
	void *sigargs;
	for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
		if (args.args == NULL) {
			sigargs = NULL;
		} else {
			sigargs = calloc(args.length,1);
			memcpy(sigargs,args.args,args.length);
		}
		cur->handler(signal,(signal << (sizeof(int) * 8 / 2)) + i,sigargs);

		++i;
	}
	return 0;
}
