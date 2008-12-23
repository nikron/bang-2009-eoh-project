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
/// \def The default port number.
#define DEFAULT_PORT "7878"

/**
 * \param port The port the server should start at.
 * \param start_server If true, the server thread is started on init.
 *
 * \brief Initalizes the net part of the library.
 */
void BANG_net_init(char *port,char start_server);

/**
 * \brief Closes and frees the net part of the library.
 */
void BANG_net_close();

/**
 * \param port The port that BANG should bind to.
 *
 * \brief A networking thread that allows others to connection to this machine.  It creates
 * slave connection threads.
 * TODO: Currently only one of theses works at a time, should the program be allowed to sit on
 * any number of ports?
 */
void* BANG_server_thread(void *port);

/**
 * \param addr The address that the thread should connect to.
 *
 * \brief This thread connects to another bang-machine and creates a slave thread.
 */
void* BANG_connect_thread(void *addr);

/**
 *
 * \brief Status of the server.
 */
char BANG_is_server_running();

/**
 * \param server_port Starts server on server_port or previously chosen one if NULL.
 *
 * \brief Starts a server on the specified port.  Only one server can be running at a time.
 * Unspecified behavior if you try to call this function more than once repeatedly.
 */
void BANG_server_start(char *server_port);


/**
 * \brief Attempts to stop the server.
 */
void BANG_server_stop();
#endif
