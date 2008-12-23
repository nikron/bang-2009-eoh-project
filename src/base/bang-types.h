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

///The structure that represents a module for the program.
typedef struct _BANG_module {
	///The name of the module.
	char *module_name;
	///The module initialize method.
	int (*module_init)();
	///The module run method.
	void (*module_run)();
} BANG_module;

///This is the structure that is sent to each signal handler.
typedef struct {
	///Arguments you want to send to signal handlers.
	void *args;
	///Lenth of the arguments.
	int length;
} BANG_sigargs;

/// A signal handler function, it receives the BANGSIGNUM, and arguments depending on the signal.
typedef void (*BANGSignalHandler)(int,int,void*);

/// The signals that can be sent by library.
enum BANG_signals {
	///Success signals.
	BANG_BIND_SUC = 0,
	BANG_PEER_ADDED,
	BANG_PEER_REMOVED,
	BANG_RUNNING_MODULE,

	///Error signals.
	BANG_BIND_FAIL,
	BANG_GADDRINFO_FAIL,
	BANG_CONNECT_FAIL,
	BANG_LISTEN_FAIL,
	BANG_MODULE_ERROR,

	//To be acted on signals.
	BANG_PEER_CONNECTED,
	BANG_PEER_DISCONNECTED,

	///Don't know if we can implment this signals
	BANG_CLOSE_ALL,

	///The number of signals.  All new signals should be above this line.
	BANG_NUM_SIGS
} BANG_signal;

#endif
