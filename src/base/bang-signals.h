/**
 * \file bang-signals.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Allows the library to send out signals, and for the applications to
 *	receive signals.
 */

/**
 * \page BANG Signals
 *
 * Bang signals are fairly simple.  When the library needs to report an event,
 * it will be reported using this signal subsystem.  Events can be anything from
 * from the network binding, a client connecting, or a module getting loaded.
 *
 * Usage is simple, install a BANGSignalHandler to catch a signal defined in
 * bang-types.h.  Once the handler is done doing its job, it should use
 * BANG_acknowledge_signal to be able to receive more signals.  If you are
 * writing a library function, use BANG_send_signal to send out a signal to
 * all of its handlers.  Be sure to document what its arguments are.
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
int BANG_install_sighandler(int signal, BANGSignalHandler handler);

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
 * \param args The arguements to the handlers which are called.
 *
 * \brief Calls the handlers of signal with args.
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
