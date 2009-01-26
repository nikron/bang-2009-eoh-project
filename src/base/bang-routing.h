/**
 * \file bang-routing.h
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Routes jobs and assertions of authority between
 * modules using the communication infrastructre.
 */
#ifndef __BANG_ROUTING_H
#define __BANG_ROUTING_H
#include<uuid/uuid.h>
#include"bang-module.h"

/**
 * \param authority The module-peer sending the job.
 * \param peer The module-peer to route to.
 * \param job The job to route to the module-peer.
 *
 * Gets a job to the a module, whever it may be as long
 * as it is valid.
 */
void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job);

/**
 * \param authority The authority sending the job.
 * \param peers A null termianted list of routes.
 * \param job The job to route.
 *
 * \brief Routes the job to all peers in the uuid list.
 */
void BANG_route_job_to_uuids(uuid_t authority, uuid_t *peers, BANG_job *job);

/**
 * \param auth The route to send a finished job through.
 * \param job The finished job to route.
 *
 * \brief Routes a finished job through a route.
 */
void BANG_route_finished_job(uuid_t auth, uuid_t peer, BANG_job *job);

/**
 * \param uuid The route to request a job from.
 *
 * \brief Requests a job from a peer through route.
 */
void BANG_route_request_job(uuid_t peer, uuid_t authority);

/**
 * \param authority Route to the authority.
 * \param peer Peer that needs that needs to respect authority.
 *
 * \brief Routes an assertation of authority to uuid.
 */
void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer);

/**
 * \param uuid Route to send debug message.
 * \param message The message that will be printed out on the remote end.
 *
 * \brief Sends a message to along the route.
 */
void BANG_route_send_message(uuid_t uuid, char *message);

/**
 * \param uuid The route to a peer.
 *
 * \return A peer id of the route if valid.  Otherwise -1.
 *
 * \brief Gets the peer id from the route.
 */
int BANG_route_get_peer_id(uuid_t uuid);

/**
 * \param uuids The routes to peers, null terminated.
 *
 * \return NULL terminated array of peer ids.
 *
 * \brief Returns peer_ids not on the list.
 * memory: You must take care of it.
 */
int** BANG_not_route_get_peer_id(uuid_t *uuids);

/**
 * \param peer A running module peer.
 * \param new_peer A new peer for the module peer.
 *
 * \brief Tells the peer that a new peer is on their network.
 */
void BANG_route_new_peer(uuid_t peer, uuid_t new_peer);

/**
 * \param peer A running module peer.
 * \param old_peer A peer being remvoed from the network.
 *
 * \brief Tells the module that they are losing a peer.
 */
void BANG_route_remove_peer(uuid_t peer, uuid_t old_peer);

/**
 * \param module The module to register with the uuid.
 *
 * \brief Registers a module at a uuid and returns that
 * uuid, inserts the uuid into the module.
 */
void BANG_register_module_route(BANG_module *module);

/**
 * \param uuid The uuid that the remote end will register as.
 * \param peer The peer where the remote module is located at.
 * \param module_name The name of the remote module.
 * \param module_version The name of the remote version.
 *
 * \brief Registers a remote module route.
 */
void BANG_register_peer_route(uuid_t uuid, int peer, char *module_name, unsigned char* module_version);

/**
 * \param uuid The identifier to deregister.
 *
 * \brief Deregisters a uuid.
 */
void BANG_deregister_route(uuid_t uid);

/**
 * \brief Starts the routing part of the library.
 */
void BANG_route_init();

/**
 * \brief Frees and stops the routing part of the library.
 */
void BANG_route_close();
#endif
