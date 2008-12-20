#include"bang-signals.h"
#include"bang-types.h"
#include<pthread.h>
#include<semaphore.h>
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
		sem_init(&(signal_handlers[signal])->signal_semaphore,0,0);
		signal_handlers[signal]->next = NULL;
		return 0;
	} else {
		signal_node *cur;
		for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
			if (cur->next == NULL) {
				cur->next =(signal_node*) malloc(sizeof(signal_node));
				cur->next->handler = handler;
				sem_init(&(cur->next->signal_semaphore),0,0);
				cur->next->next = NULL;
				return 0;
			}
		}
	}
	return -1;
}

typedef struct {
	signal_node *signode;
	int signum;
	void *handler_args;
} send_signal_args;

void* thread_send_signal(void *args) {
	send_signal_args *sigargs = (send_signal_args*)args;
	sem_wait(&(sigargs->signode->signal_semaphore));
	sigargs->signode->handler(sigargs->signum,0,args);
	free(args);
	return NULL;
}

int BANG_send_signal(int signal, void *args) {
	signal_node *cur;
	pthread_t signal_threads;
	send_signal_args *sigargs;
	for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
		sigargs = (send_signal_args*) calloc(1,sizeof(send_signal_args));
		sigargs->signode = cur;
		sigargs->signum = signal;
		sigargs->handler_args = args;

		if (!pthread_create(&signal_threads,NULL,&thread_send_signal,(void*)sigargs)) {
			return -1;
		} else {
			pthread_detach(signal_threads);
		}
	}
	return 0;
}
