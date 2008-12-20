/**
 * \file bang-signals.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Allows the library to send out signals, and for the applications to
 *	receive signals.
 */
#ifndef __BANG_SIGNALS_H
#define __BANG_SIGNALS_H

#include"bang-types.h"
/**
 * \param signal The signal that you want to catch.
 * \param handler Function that handles the signal.
 *
 * \brief Installs a signal such that whenever said signal gets generated
 *	handler gets called with the appropriate arguments.
 */
int BANG_install_sighandler(int signal, BANGSignalHandler *handler);

/**
 * \param signal The signal type to acknowledge.
 * \param sig_id The id of the signal
 *
 * \brief Acknowledges that the signal has been received, and that
 * the function is ready to take another.
 */
void BANG_acknowledge_signal(int signal, int sig_id);


/**
 * \param signal The signal number which to send.
 * \parman args The arguements to the handlers which are called.
 *
 * \breif Calls the handlers of signal with args.
 */
int BANG_send_signal(int signal, void *args);

/**
 *
 * \brief Initializes the signal porition of the library.
 */
void BANG_sig_init();

/**
 *
 * \brief Frees and stops signals from working.
 */
void BANG_sig_close();
#endif
