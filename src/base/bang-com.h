/**
 * \file bang-com.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Functions to communicate between peers using a master slave system.
 * */
#ifndef __BANG_COM_H
#define __BANG_COM_H
/**
 * \param args
 *
 * \brief The master of all the slaves threads.  It should make sure their request are fulfilled, and
 * send out requests using its slaves.
 */
void* BANG_master_thread(void *args);

/**
 * \param socket The connection with the peer.
 *
 * \brief A slave connection thread.  This thread symbolizes a connection with another peer.
 */
void* BANG_slave_thread(void *socket);

/**
 * \param socket The slave client socket.
 *
 * \brief Creates a slave thread using the socket gotten from the signal.
 */
void BANG_connection_signal_handler(int signal,int sig_id,void* socket);
#endif
