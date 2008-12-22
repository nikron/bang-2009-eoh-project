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
void BANG_peer_added(int signal,int sig_id,void* socket);

/**
 * \param peer_id The peer to be remove
 *
 * \brief Catches BANG_PEER_TO_BE_REMOVED, and remove that peer, and emits
 */
void BANG_peer_removed(int signal,int sig_id,void *peer_id);

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
