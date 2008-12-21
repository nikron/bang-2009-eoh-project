/**
 * \file bang-com.c
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Implements "the master of slave peer threads" model.
 * */
#include"bang-com.h"
#include"bang-signals.h"
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>

/**
 * \page Master-Slave Model
 *
 * A master thread should service requests from its slave-peer threads, and then request
 * things of them.  (These requests would be made by the modules).
 */

///Represents one of our peers.
typedef struct {
	pthread_t *thread;
	int peer_id;
} peer_slave;

int num_peers = 0;
peer_slave *peer_slaves;

///TODO: Question to be answered, how do these communicate?  BANG_signals?

void* BANG_master_thread(void *args) {
	///TODO:Implement this whole thing..
	return NULL;
}

void* BANG_slave_thread(void *socket) {
	///TODO: Implement this whole idea.. uhh any volunteers?
	while (1) {}
	close(*((int*)socket));
	free(socket);
	return NULL;
}

void BANG_connection_signal_handler(int signal,int sig_id,void* socket) {
	peer_slaves = (peer_slave*) realloc(peer_slaves,(++num_peers) * sizeof(peer_slave));
	peer_slaves[num_peers - 1].thread = (pthread_t*) calloc(1,sizeof(pthread_t));
	peer_slaves[num_peers - 1].peer_id = num_peers - 1;
	pthread_create(peer_slaves[num_peers - 1].thread,NULL,&BANG_slave_thread,socket);
	BANG_acknowledge_signal(signal,sig_id);
}
