/**
 * \file bang-module-api.c
 * \author Nikhil Bysani
 * \date December 24, 2009
 *
 * \brief An api for the modules so they can interface
 * with the program, and more importantly its peers.
 */

#ifndef __BANG_MODULE_API_H
#define __BANG_MODULE_API_H
/**
 * \param The message you want to send to all peers.
 *
 * \brief Prints a debugging message on all peers.
 */
void BANG_debug_on_all_peers(char *message);

typedef struct {
	void (*BANG_debug_on_all_peers) (char *message);
} BANG_api;
#endif
