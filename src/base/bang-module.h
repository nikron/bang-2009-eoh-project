/**
 * \file bang-module.h
 * \author Nikhil Bysani
 * \date December 21, 2008
 *
 * \brief A module gets loaded and run from this.  The module calls functions in this
 * to get things done.
 */
#ifndef __BANG_MODULE_H
#define __BANG_MODULE_H
#include"bang-module-api.h"
#include"bang-types.h"

/**
 * \param path File path to a module that can be loaded.
 *
 * \brief Loads a module.
 */
BANG_module* BANG_load_module(char *path);

/**
 * \param module The module to unload.
 *
 * \brief Unloads a module.
 */
void BANG_unload_module(BANG_module *module);

/**
 * \param symbol The symbol you want to get.
 *
 * \brief Tries to fetch a symbol from the BANG_module, if
 * unsuccessful, returns NULL
 */
void* BANG_get_symbol(BANG_module *module, char *symbol);

/**
 * \brief Runs a module.
 */
void BANG_run_module(BANG_module *module);

/**
 * \param module The module of the peer.
 * \param job The job that is being sent to the module.
 * \param auth The peer who sent the job to the module.
 * \param peer The uuid of the module (hopefully.
 *
 * \brief If the peer and auth uuid checks out, then the job get sent to the module's
 * callback.
 */
void BANG_module_callback_job(const BANG_module *module, BANG_job *job, uuid_t auth, uuid_t peer);

/**
 * \param module The module of the peer.
 * \param auth The uuid of the authority saying a job is available.
 * \param peer The uuid of the peer being told that a job is available at auth.
 *
 * \brief A callback to tell peer with module that there is a job available held
 * by auth.
 */
void BANG_module_callback_job_available(const BANG_module *module, uuid_t auth, uuid_t peer);

/**
 * \param module The module of the auth.
 * \param auth The authority getting asked for a job.
 * \param peer The peer requesting a job.
 *
 * \brief A callback to tell the authority that the uuid peer wants a job.
 */
void BANG_module_callback_job_request(const BANG_module *module, uuid_t auth, uuid_t peer);

/**
 * \param module The module of the auth.
 * \param job The finished job.
 * \param auth The uuid of the module authority.
 * \param peer The uuid of the peer sending the module.
 *
 * \brief Sends a finished job to the module.
 */
void BANG_module_callback_job_finished(const BANG_module *module, BANG_job *job, uuid_t auth, uuid_t peer);

/**
 * \param module The module getting a new peer.
 * \param uuid The uuid of the module.
 * \param new_peer The uuid of the new peer.
 *
 * \brief Informs the module that it has a new peer.
 */
void BANG_module_new_peer(const BANG_module *module, uuid_t peer, uuid_t new_peer);

/**
 * \param module The module losinga  peer.
 * \param peer The uuid of the module.
 * \param old_peer The peer that the module is losing.
 *
 * \brief Informs the module that is losing a peer.
 */
void BANG_module_remove_peer(const BANG_module *module, uuid_t peer,uuid_t old_peer);
#endif
