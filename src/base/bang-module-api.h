/**
 * \file bang-module-api.h
 * \author Nikhil Bysani
 * \date December 24, 2008
 *
 * \brief An api for the modules so they can interface
 * with the program, and more importantly its peers.
 */

#ifndef __BANG_MODULE_API_H
#define __BANG_MODULE_API_H
/**
 * \param The message you want to send to all peers.
 *
 * \brief Prints a debugging message on all peers.
 */
void BANG_debug_on_all_peers(char *message);

/**
 * \brief Tries to get a module as many peers as possible.
 */
void BANG_get_me_peers();

/**
 * \return Retuns the number of peers currently active. (Peers being peers
 * who are running your module.
 */
int BANG_number_of_active_peers();

/**
 * \return Returns your peer number.  (This is not a peer_id, it is a number
 * to identify you to peers running the module).
 *
 * Discussion:  Should this number be passed to the init function?
 */
int BANG_get_my_id();

/**
 * The api for the modules to use.
 */
typedef struct {
	/**
	 * Refrence to BANG_debug_on_all_peers.
	 */
	void (*BANG_debug_on_all_peers) (char *message);
	void (*BANG_get_me_peers) ();
	int (*BANG_number_of_active_peers) ();
	int (*BANG_get_my_id) ();
} BANG_api;
#endif
