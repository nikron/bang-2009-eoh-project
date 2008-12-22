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
 * \brief A networking thread that allows others to connection to this machine.  It creates
 * slave connection threads.
 */
void* BANG_server_thread(void *port);

/**
 * \param addr The address that the thread should connect to.
 *
 * \brief This thread connects to another bang-machine and creates a slave thread.
 */
void* BANG_connect_thread(void *addr);

/**
 * \brief Closes and frees the net part of the library.
 */
void BANG_net_close();

/**
 * \param port The port the server should start at.
 * \param start_server If true, the server thread is started on init.
 *
 * \brief Initalizes the net part of the library.
 */
void BANG_net_init(char *port,char start_server);
#endif
