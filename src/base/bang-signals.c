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
 * A list of handlers for each signal is stored in linked list.
 */
struct _signal_node {
	/**
	 * A handler for the signal.
	 */
	BANGSignalHandler handler;
	/**
	 * Next node in the list.
	 */
	struct _signal_node *next;
};

/**
 * A simple typedef of _signal_node.
 */
typedef struct _signal_node signal_node;

/**
 * Another read-writer problem, multiple threads can send out the same signal at the same time, but only one thread
 * should be allowed add a signal handler.
 */
BANG_rw_syncro *(sig_locks[BANG_NUM_SIGS]);

/**
 * \brief The handlers for each of the signals is kept in a linked list stored
 *  in an array which is index by the signal's number
 */
static signal_node **signal_handlers;

/**
 * \param head The head of the signal_node list.
 *
 * \brief Frees a signal node linked list.
 */
static void recursive_sig_free(signal_node *head);

/**
 * \param signal The signal whose lock you want to acquire.
 *
 * \brief Acquires a read lock on a signal.
 */
static void acquire_sig_lock(int signal);

/**
 * \param signal The signal whose lock you want to release.
 *
 * \brief Releases a read lock on a signal.
 */
static void release_sig_lock(int signal);

static void recursive_sig_free(signal_node *head) {
	if (head == NULL) return;
	if (head->next != NULL) {
		recursive_sig_free(head->next);
	}
	free(head);
	head = NULL;
}

static void acquire_sig_lock(int signal) {
	BANG_read_lock(sig_locks[signal]);
}

static void release_sig_lock(int signal) {
	BANG_read_unlock(sig_locks[signal]);
}

void BANG_sig_init() {
	int i;

	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		signal_handlers[i] = NULL;
		sig_locks[i] = new_BANG_rw_syncro();
	}
}

void BANG_sig_close() {
#ifdef BDEBUG_1
	fprintf(stderr,"BANG sig closing.\n");
#endif
	int i;
	for (i = 0; i < BANG_NUM_SIGS; ++i) {
		recursive_sig_free(signal_handlers[i]);
		free_BANG_rw_syncro(sig_locks[i]);
	}
}

int BANG_install_sighandler(int signal, BANGSignalHandler handler) {
	/* We need to get a lock on the signal so that people aren't creating
	 * more that one signal at a time */
	BANG_write_lock(sig_locks[signal]);

	if (signal_handlers[signal] == NULL) {
		/* Create a head node if there is none. */
		signal_handlers[signal] = (signal_node*) malloc(sizeof(signal_node));
		signal_handlers[signal]->handler = handler;
		signal_handlers[signal]->next = NULL;

		BANG_write_unlock(sig_locks[signal]);
		return 0;
	} else {
		signal_node *cur;
		/* Append the handler if there is a head. */
		for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
			if (cur->next == NULL) {
				cur->next =(signal_node*) malloc(sizeof(signal_node));
				cur->next->handler = handler;
				cur->next->next = NULL;

				BANG_write_unlock(sig_locks[signal]);
				return 0;
			}
		}
	}
	/* How could it possibly come here!?1?!? */
	BANG_write_unlock(sig_locks[signal]);
	return -1;
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

int BANG_send_signal(int signal, BANG_sigargs *args, int num_args) {

#ifdef BDEBUG_1
	fprintf(stderr,"Sending out the signal %d.\n",signal);
	fprintf(stderr,"The signal_node is %p.\n",signal_handlers[signal]);
#endif

	signal_node *cur;
	send_signal_args *thread_args;
	pthread_t signal_thread;
	int i = 0;


	/* Grab a read lock so the amount of signal handlers does not change */
	acquire_sig_lock(signal);

	for (cur = signal_handlers[signal]; cur != NULL; cur = cur->next) {
		/* Used to transfer data to the thread. */
		thread_args = (send_signal_args*) calloc(1,sizeof(send_signal_args));

		thread_args->handler_args = calloc(num_args,sizeof(void*));

		/* Each signal handler must have its own copy of the arguments. */
		for (i = 0; i < num_args; ++i) {
			thread_args->handler_args[i] = calloc(args[i].length,1);
			memcpy(thread_args->handler_args[i],args[i].args,args[i].length);
		}

		thread_args->signal = signal;
		thread_args->handler = cur->handler;
		thread_args->num_handler_args = num_args;

		pthread_create(&signal_thread,NULL,threaded_send_signal,(void*)thread_args);
		/* We wont try to keep track of these threads */
		pthread_detach(signal_thread);
	}

	release_sig_lock(signal);
	return 0;
}
