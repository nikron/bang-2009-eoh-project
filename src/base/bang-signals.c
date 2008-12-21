#include"bang-signals.h"
#include"bang-types.h"
#include<errno.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>

struct _signal_node {
	BANGSignalHandler *handler;
	sem_t signal_semaphore;
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

int BANG_install_sighandler(int signal, BANGSignalHandler *handler) {
	if (signal_handlers[signal] == NULL) {
		signal_handlers[signal] = (signal_node*) malloc(sizeof(signal_node));
		signal_handlers[signal]->handler = handler;
		sem_init(&(signal_handlers[signal])->signal_semaphore,0,1);
		signal_handlers[signal]->next = NULL;
		return 0;
	} else {
		signal_node *cur;
		for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
			if (cur->next == NULL) {
				cur->next =(signal_node*) malloc(sizeof(signal_node));
				cur->next->handler = handler;
				sem_init(&(cur->next->signal_semaphore),0,1);
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

void* thread_send_signal(void *args) {
	send_signal_args *sigargs = (send_signal_args*)args;
#ifdef BDEBUG_1
	fprintf(stderr,"Going to wait for handler %p with %p.\n",sigargs->signode->handler,&(sigargs->signode->signal_semaphore));
#endif
	sem_wait(&(sigargs->signode->signal_semaphore));
#ifdef BDEBUG_1
	fprintf(stderr,"Done waiting for handler %p.\n",sigargs->signode->handler);
#endif
	sigargs->signode->handler(sigargs->signal,sigargs->sig_id,args);
	free(args);
	return NULL;
}

int BANG_send_signal(int signal, void *args) {
#ifdef BDEBUG_1
	fprintf(stderr,"Sending out the signal %d.\n",signal);
	fprintf(stderr,"The signal_node is %p.\n",signal_handlers[signal]);
#endif

	signal_node *cur;
	pthread_t signal_threads;
	send_signal_args *sigargs;
	int i = 0;
	for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
		sigargs = (send_signal_args*) calloc(1,sizeof(send_signal_args));
		sigargs->signode = cur;
		sigargs->signal = signal;
		sigargs->sig_id = (signal << (sizeof(int) * 8 / 2)) + i;
		sigargs->handler_args = args;

#ifdef BDEBUG_1
		fprintf(stderr,"Sending out signal to handler %p.\n",cur->handler);
#endif
		if (pthread_create(&signal_threads,NULL,&thread_send_signal,(void*)sigargs) != 0) {
#ifdef BDEBUG_1
			fprintf(stderr,"Pthread create error.\n");
#endif
			return -1;
		} else {
			pthread_detach(signal_threads);
		}
		++i;
	}
	return 0;
}


void BANG_acknowledge_signal(int signal, int sig_id) {
	signal_node *cur = signal_handlers[sig_id >> (sizeof(int) * 8 /2)];
	int i = 0, target = sig_id << (sizeof(int) * 8 /2) >> (sizeof(int) * 8 /2);
	for (; cur != NULL; cur = cur->next) {
		if (i == target) {
#ifdef BDEBUG_1
			fprintf(stderr,"Signal semaphore %d has been posted.\n",target);
#endif
			sem_post(&(cur->signal_semaphore));
			break;
		}
		++i;
	}
}
