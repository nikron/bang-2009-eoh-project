/**
 * \file bang-core.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief This defines the main public high level interface to the BANG project.  This is mainly accomplished
 * through use of BANG_init and BANG_close.
 */
#ifndef __BANG_CORE_H
#define __BANG_CORE_H

#include"bang-net.h"
#include"bang-types.h"
#include"bang-signals.h"

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
 * \brief Potientally finds peers using zero-conf.  This is a candidate to be moved to another file.
 */
void* BANG_find_peers();
#endif
