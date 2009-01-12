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

typedef struct {
	void *data;
	unsigned int length;
	/**
	 * The id of the peer who gave you this job.
	 */
	int authority;
} BANG_job;

/**
 * Callbacks that the module _has_ to provide
 * to know about events.
 *
 * Returned from the module init function.
 */
typedef struct {
	void (*outgoing_job) (int);
	void (*incoming_job) (BANG_job);
	void (*peer_added) (int);
	void (*peer_removed) (int);
} BANG_callbacks;

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
 * \return Returns the number of peers currently active. (Peers being peers
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
 * \brief Tells all your peers that peer id has a job waiting.
 */
void BANG_assert_authority(int id);

/**
 * \param id The peer to get a job from.
 * \param blocking If true, the callback will be used instead.
 *
 * \return A job for you to act upon.
 *
 * \brief If non-null, there is a job waiting for the module.
 * Else, an error or non blocking happened.
 *
 */
BANG_job* BANG_request_job(int id, char blocking);

int BANG_finished_request(BANG_job job);

void BANG_send_job(int id, BANG_job job);

/**
 * The api for the modules to use.
 */
typedef struct {
	/**
	 * Reference to BANG_debug_on_all_peers.
	 */
	void (*BANG_debug_on_all_peers) (char *message);
	void (*BANG_get_me_peers) ();
	int (*BANG_number_of_active_peers) ();
	int (*BANG_get_my_id) ();
	void (*BANG_assert_authority) (int id);
	BANG_job* (*BANG_request_job) (int id, char blocking);
	int (*BANG_finished_request) (BANG_job job);
	void (*BANG_send_job) (int id, BANG_job job);
} BANG_api;
#endif
