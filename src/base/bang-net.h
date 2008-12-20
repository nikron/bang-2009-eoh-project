/**
 * \file bang-net.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Public interface for the networking part of BANG.
 * */
#ifndef __BANG_NET_H
#define __BANG_NET_H

/// \def Maximum number of waiting connections allowed.  The user should be able to set this, but currently can not.
#define MAX_BACKLOG 70
#define DEFAULT_PORT "7878"

/**
 * \param port The port that BANG should bind to.
 *
 * \brief A networking thread that allows others to connection to this machine.
 */
void* BANG_network_thread(void *port);
#endif
