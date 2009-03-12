/**
 * \file bang-module-registry.h
 * \author Nikhil Bysani
 * \date March 11, 2009
 *
 * \brief Keeps track of all running modules.
 */

#ifndef __BANG_MODULE_REGISTRY_H
#define __BANG_MODULE_REGISTRY_H

typedef struct {
	BANG_module *module;
	char started;
} registry_slot;

/**
 * \param path The file system path to the module.
 * \param module_name The function puts the name of the module here.
 * \param module_version The function puts the version of the module here.
 *
 * \brief Creates a new module in the registry, and tells you its name and version.
 */
void BANG_new_module(char *path, char **module_name, unsigned char **module_version);

/**
 * \param module_name The name of the module you are trying to get.
 * \param module_version The version of the module you are trying to get.
 *
 * \brief Gets you the internal representation of a module.
 *
 * \return A BANG_module with the matching name and version.
 */
BANG_module* BANG_get_module(char *module_name, unsigned char *module_version);

/**
 * \param module_name The name of the module to start.
 * \param module_version The version of the module to start.
 *
 * \brief Starts a module of the given name and version.
 *
 * \return Returns 0 on success, something else on error.
 */
int BANG_run_module_in_registry(char *module_name, unsigned char *module_version);

/**
 * \param module_name The name of the module that is running.
 * \param module_version The version of the module that is running.
 * \param new_peer The new peer of the module.
 *
 * \brief If the given module is running, tells it about the new peer.
 */
void BANG_module_inform_new_peer(char *module_name, unsigned char *module_version, uuid_t new_peer);

void BANG_module_registry_init();
void BANG_module_registry_close();

#endif
