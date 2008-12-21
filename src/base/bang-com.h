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
 * \param self_info Information about the peer.
 *
 * \brief A peer connection thread.
 */
void* BANG_peer_thread(void *self_info);

/**
 * \param socket The slave client socket.
 *
 * \brief Creates a slave thread using the socket gotten from the signal.
 */
void BANG_connection_signal_handler(int signal,int sig_id,void* socket);
#endif
