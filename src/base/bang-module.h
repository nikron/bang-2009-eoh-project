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

/**
 * \param path File path to a module that can be loaded.
 *
 * \brief Loads a module.
 */
int BANG_load_module(char *path);
#endif
