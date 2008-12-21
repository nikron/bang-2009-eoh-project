/**
 * \file bang-module.h
 * \author Nikhil Bysani
 * \date December 21, 2009
 *
 * \brief A module gets loaded and run from this.  The module calls functions in this
 * to get things done.
 */
#ifndef __BANG_MODULE_H
#define __BANG_MODULE_H
#include"bang-types.h"

/**
 * \param path File path to a module that can be loaded.
 *
 * \brief Loads a module.
 */
BANG_module* BANG_load_module(char *path);


/**
 * \brief Runs a module.
 */
void BANG_run_module(BANG_module *module);

#endif
