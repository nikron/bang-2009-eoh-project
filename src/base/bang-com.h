/**
 * \file bang-com.h
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Functions to communicate between peers using a master slave system.
 * */
#ifndef __BANG_COM_H
#define __BANG_COM_H
/**
 * \page Peer Implementation
 * Outgoing requests are serviced by iterating through the peers and sending out requests using the
 * peers list and corresponding structure.  There are two threads for each peer.  One to manage
 * incoming requests, and one to manage outgoing requests.
 */
#include"bang.h"

/**
 * \brief Initializes the bang-com part of BANG.
 */
void BANG_com_init();

/**
 * \brief Closes down all the peer threads, destroy the semaphores, and
 * frees all the used memory.
 */
void BANG_com_close();

/**
 * \param self_info Information about the peer.
 *
 * \brief A peer connection thread.
 */
void* BANG_read_peer_thread(void *self_info);

/**
 * \param self_info Information about the peer.
 *
 * \brief A peer connection thread.
 */
void* BANG_write_peer_thread(void *self_info);

/**
 * \param socket The slave client socket.
 *
 * \brief Creates a slave thread using the socket gotten from the signal.
 */
void BANG_catch_add_peer(int signal, int sig_id, void *socket);

/**
 * \param peer_id The peer to be remove
 *
 * \brief Catches BANG_PEER_TO_BE_REMOVED, and remove that peer, and emits
 */
void BANG_catch_remove_peer(int signal, int sig_id, void *peer_id);

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
void BANG_request_peer_id(int peer_id, BANG_request request);

/**
 * \param request The request to add.
 *
 * \brief Adds request to all peers.
 */
void BANG_request_all(BANG_request request);

void BANG_catach_request_all(int signal, int signum, void *vrequest);

/**
 * \param peer_id Peer to remove.
 *
 * \brief Removes peer_id, and it associated threads from existence, and emits
 * a BANG_PEER_REMOVED.  If you want the peer to removed in a different thread, send
 * out a BANG_PEER_TO_BE_REMOVED
 */
void BANG_remove_peer(int peer_id);

/**
 * \param socket
 *
 * \brief Adds a peer with socket, creates its threads.  If you want other functions to
 * be able to catch this, remember to use a signal.
 */
void BANG_add_peer(int socket);


/**
 * \param peer_id The id of the peer thread.
 *
 * \brief Gets the current key of the peer id (the place it is located in the keys array).
 */
int BANG_get_key_with_peer_id(int peer_id);
#endif
