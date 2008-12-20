/**
 * \file core.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief This defines the main _public_ interface to the BANG project.  This is mainly accomplished
 * through use of BANG_init, BANG_close, and BANG_install_sighandler.
 */
#ifndef __CORE_H
#define __CORE_H

#include"bang-net.h"
#include"bang-types.h"

/**
 * \param argc The number of arguments of the program.
 * \param argv The arguments to the program.
 *
 * \brief Initializes the core library of BANG
 */
void BANG_init(int *argc, char **argv);

/**
 * \brief Closes down the project and frees all of its resources.
 */
void BANG_close();

/**
 * \param path File path to a module that can be loaded.
 *
 * \brief Loads a module.
 */
int BANG_load_module(char *path);

/**
 * \param signal The signal that you want to catch.
 * \param handler Function that handles the signal.
 *
 * \brief Installs a signal such that whenever said signal gets generated
 * 	handler gets called with the appropriate arguments.
 */
int BANG_install_sighandler(int signal, BANGSignalHandler handler);

/**
 * \brief Potientally finds peers using zero-conf.  This is a candidate to be moved to another file.
 */
void* BANG_find_peers();
#endif
