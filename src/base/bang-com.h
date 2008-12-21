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
 * \brief Intializes the bang-com part of BANG.
 */
void BANG_com_init();

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
void BANG_peer_signal_handler(int signal,int sig_id,void* socket);

/**
 *
 * \return A new unique peer id.
 */
int BANG_new_peer_id();


/**
 * \param peer_id The peer you want to add a request.
 * \param request The request you want to add to the peer.
 *
 * \brief Adds a request to the peer_id peer.
 */
///TODO: Figure out the structure for the request
void BANG_add_request_to_peer(int peer_id,void *request); 

/**
 * \param request The request to add.
 *
 * \brief Adds request to all peers.
 */
void BANG_add_request_all(void *request);

/**
 * \param peer_id Peer to remove.
 *
 * \brief Removes peer_id, and it associated thread for existance.
 */
#endif
