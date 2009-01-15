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

BANG_module_info* new_BANG_module_info(BANG_module *module);

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

#endif
