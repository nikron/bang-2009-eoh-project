/**
 * \file bang-types.h
 * \author Nikhil Bysani
 * \date December 20, 2009
 *
 * \brief Defines commonly used types in the BANG library.
 * 
 */
#ifndef __BANG_TYPES_H
#define __BANG_TYPES_H

/// A signal handler function, it receives the BANGSIGNUM, and arguments depending on the signal.
typedef void BANGSignalHandler(int,int,void *args);

/// The number of signals that can be sent by the library.
#define BANG_NUM_SIGS 5

/// The signals that can be sent by library.
enum BANG_signals {
	BANG_BIND_SUC = 0,
	BANG_BIND_FAIL,
	BANG_GADDRINFO_FAIL,
	BANG_LISTEN_FAIL,
	BANG_CLIENT_CONNECTED
} BANG_signal;

#endif
