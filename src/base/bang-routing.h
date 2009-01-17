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
 * \param uuid The module-peer to route to.
 * \param job The job to route to the module-peer.
 *
 * Gets a job to the a module, whever it may be as long
 * as it is valid.
 */
void BANG_route_job(uuid_t uuid, BANG_job *job);

/**
 * \param uuids A null termianted list of routes.
 * \param job The job to route.
 *
 * \brief Routes the job to all peers in the uuid list.
 */
void BANG_route_job_to_uuids(uuid_t *uuids, BANG_job *job);

/**
 * \param uuid The route to send a finished job through.
 *
 * \brief Routes a finished job through a route.
 */
void BANG_route_finished_job(uuid_t uuid, BANG_job *job);

/**
 * \param uuid The route to request a job from.
 *
 * \brief Requests a job from a peer through route.
 */
void BANG_route_request_job(uuid_t uuid);

/**
 * \param authority Route to the authority.
 * \param peer Peer that needs that needs to respect authority.
 *
 * \brief Routes an assertation of authority to uuid.
 */
void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer);

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
void BANG_register_peer_route(uuid_t uuid, int peer, char *module_name, char* module_version);
#endif
