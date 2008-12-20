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

#define BANG_NUM_SIGS 1
enum BANG_signals {
	BANG_BIND_SUC = 0
} BANG_signal;

#endif
