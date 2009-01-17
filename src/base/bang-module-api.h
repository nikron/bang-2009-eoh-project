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

#include"bang-module-api.h"
#include<uuid/uuid.h>

/**
 * Information about a module's peers.  A module needs
 * to call its api functions with this to work properly.
 * The module should *not* touch this structure even
 * though it is fairly transparent.
 */
typedef struct {
	/**
	 * Peers of the module.  They are known to the
	 * module as their index + 1.  The actual contents
	 * of the int is a peer_id
	 */
	uuid_t *uuids;
	/**
	 * The number of peers that this module has.
	 */
	int peer_number;
	/**
	 * The id of module running.  Currently is always
	 * 0, but modules shouldn't depend on that.
	 */
	int my_id;
} BANG_module_info;

typedef struct {
	void *data;
	unsigned int length;
	/**
	 * For use by the module, if it wants
	 * this.
	 */
	int job_number;
	/**
	 * The peer doing the work.
	 */
	int peer;
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
	/**
	 * Callback for when jobs are available for you to request
	 * \param int The id of the peer with jobs available.
	 */
	void (*jobs_available) (BANG_module_info*, int);
	/**
	 * Callback for when a finished job is sent to you.
	 * \param BANG_job* The job that has been finished.
	 */
	void (*jobs_done) (BANG_module_info*, BANG_job*);
	/**
	 * Callback for when a peer requests you a job from you.
	 * \param int The id of peer requesting a job.
	 */
	void (*outgoing_job) (BANG_module_info*, int);
	/**
	 * Callback for when a job is sent to you.
	 * \param BANG_job* the job that is sent to you.
	 */
	void (*incoming_job) (BANG_module_info*, BANG_job*);
	/**
	 * Callback for when a peer is added.
	 * \param int The id of the added peer.
	 */
	void (*peer_added) (BANG_module_info*, int);
	/**
	 * Callback for when a peer is removed.
	 * \param int The id of the removed peer.
	 */
	void (*peer_removed) (BANG_module_info*, int);
} BANG_callbacks;

/**
 * \param The message you want to send to all peers.
 *
 * \brief Prints a debugging message on all peers.
 */
void BANG_debug_on_all_peers(BANG_module_info *info, char *message);

/**
 * \brief Tries to get a module as many peers as possible.
 */
void BANG_get_me_peers(BANG_module_info *info);

/**
  *\return Returns the number of peers currently active. (Peers being peers
 * who are running your module.
 */
int BANG_number_of_active_peers(BANG_module_info *info);

/**
 * \return Returns your peer number.  (This is not a peer_id, it is a number
 * to identify you to peers running the module).
 *
 * Discussion:  Should this number be passed to the init function?
 */
int BANG_get_my_id(BANG_module_info *info);

/**
 * \brief Tells all your peers that peer id has a job waiting.
 */
void BANG_assert_authority(BANG_module_info *info, int id);

void BANG_assert_authority_to_peer(BANG_module_info *info, int authority, int peer);

/**
 * \param id The peer to get a job from.
 * \param blocking If false, the callback will be used instead.
 *
 * \return A job for you to act upon.
 *
 * \brief If non-null, there is a job waiting for the module.
 * Else, an error or non blocking happened.
 *
 */
BANG_job *BANG_request_job(BANG_module_info *info, int id, char blocking);

/**
 * \param job The finished job.
 *
 * \brief Sends the job to the peer.  If the peer is you,
 * it will send it to your callback method.
 */
void BANG_finished_request(BANG_module_info *info, BANG_job *job);

/**
 * \param The peer to send your the job to.
 *
 * \brief Sends a job to a peer.
 */
void BANG_send_job(BANG_module_info *info, BANG_job *job);

/**
 * The api for the modules to use.  This will
 * be filled out with pointers to the above functions and
 * passed to the module.
 */
typedef struct {
	void (*BANG_debug_on_all_peers) (BANG_module_info *info, char *message);
	void (*BANG_get_me_peers) (BANG_module_info *info);
	int (*BANG_number_of_active_peers) ();
	int (*BANG_get_my_id) (BANG_module_info *info);
	void (*BANG_assert_authority) (BANG_module_info *info, int id);
	void (*BANG_assert_authority_to_peer) (BANG_module_info *info, int authority, int peer);
	BANG_job* (*BANG_request_job) (BANG_module_info *info, int id, char blocking);
	void (*BANG_finished_request) (BANG_module_info *info, BANG_job *job);
	void (*BANG_send_job) (BANG_module_info *info, BANG_job *job);
} BANG_api;
#endif
