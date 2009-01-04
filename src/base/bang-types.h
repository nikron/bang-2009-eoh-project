/**
 * \file bang-types.h
 * \author Nikhil Bysani
 * \date December 20, 2008
 *
 * \brief Defines commonly used types in the BANG library.
 */
#ifndef __BANG_TYPES_H
#define __BANG_TYPES_H
#include"bang-module-api.h"

#define BANG_VERSION .01

/**
 * The structure that represents a module for the program.
 */
typedef struct _BANG_module {
	/**
	 * The name of the module.
	 */
	char *module_name;
	double *module_version;
	/**
	 * The module initialize method.
	 */
	int (*module_init)(BANG_api*);
	/**
	 * The module run method.
	 */
	void (*module_run)();

	unsigned char *md;
	void *handle;
	char *path;
} BANG_module;

/**
 * This is the structure that is sent to each signal handler.
 */
typedef struct {
	/**
	 * Arguments you want to send to signal handlers.
	 */
	void *args;
	/**
	 * Length of the arguments.
	 */
	int length;
} BANG_sigargs;

/**
 * A signal handler function, it receives the BANGSIGNUM, and arguments depending on the signal.
 */
typedef void (*BANGSignalHandler)(int,int,void*);

/** 
 * The signals that can be sent by library.
 * TODO: Detail all the signals, especially their arguments.
 */
enum BANG_signals {
	///Success signals.
	BANG_BIND_SUC = 0,
	BANG_PEER_ADDED,
	BANG_PEER_REMOVED,
	BANG_RUNNING_MODULE,
	BANG_SERVER_STARTED,
	BANG_SERVER_STOPPED,

	///Error signals.
	BANG_BIND_FAIL,
	BANG_GADDRINFO_FAIL,
	BANG_CONNECT_FAIL,
	BANG_LISTEN_FAIL,
	BANG_MODULE_ERROR,

	/*
	 * To be acted on signals.
	 */
	BANG_PEER_CONNECTED,
	BANG_PEER_DISCONNECTED,
	BANG_REQUEST_ALL,

	/**
	 * Don't know if we can (will) implement this signals
	 */
	BANG_CLOSE_ALL,

	/**
	 * The number of signals.  All new signals should be above this line.
	 */
	BANG_NUM_SIGS
} BANG_signal;

/**
 *
 * \page The Protocol
 *
 * \brief The protocol is specified, like this:
 *  - First, an unsigned 4 bytes will be sent specifying what kind of communication it will be
 *  - Then, everything else will follow
 *
 *  Following are possible unsigned to be sent.
 *
 *  Communications currently can be done by this:
 *  assuming peer1, peer2
 * 
 * peer1: BANG_HELLO
 * peer2: BANG_HELLO
 *
 * ... any number of BANG_DEBUG_MESSAGE
 *
 * peer2: BANG_BYE
 *
 */

/**
 * Length of the headers.
 */
#define LENGTH_OF_HEADER 4
/**
 * Length of any lengths sent.
 */
#define LENGTH_OF_LENGTHS 4
/**
 * Length of any versions sent.
 */
#define LENGTH_OF_VERSION 8

enum BANG_headers {
	/**
	 * message:
	 * 	-BANG_HELLO (unsigned 4 bytes)
	 * 	-BANG_VERSION (8 bytes to a double)
	 * 	-length of name (unsigned 4 bytes)
	 * 	-peer name (char*)
	 */
	BANG_HELLO = 0,
	/**
	 * message:
	 * 	-BANG_DEBUG_MESSAGE (unsigned 4 bytes)
	 * 	-length of message (unsigned 4 bytes)
	 * 	-message (char*)
	 */
	BANG_DEBUG_MESSAGE,
	/**
	 * message:
	 *	-BANG_SEND_MODULE
	 *	-length of module
	 *	-module
	 * NOTE: The file, not the struct.
	 */
	BANG_SEND_MODULE,
	/**
	 * message:
	 * 	-BANG_MISMATCH_VERSION (unsigned 4 bytes)
	 * 	-our version (8 bytes)
	 */
	BANG_MISMATCH_VERSION,
	/**
	 * message:
	 * 	-BANG_BYE (unsigned 4 bytes)
	 */
	BANG_BYE,
	/**
	 * Number of headers.*/
	BANG_NUM_HEADERS
} BANG_header;

typedef struct {
	char type;
	void *request;
	unsigned int length;
} BANG_request;

enum BANG_request_types {
	/**
	 * Ask for the peer to start a sign off.
	 */
	BANG_CLOSE_REQUEST = 0,
	/**
	 * Just quit without doing anything.
	 */
	BANG_SUDDEN_CLOSE_REQUEST,
	/**
	 * do:
	 * 	-send BANG_DEBUG_MESSAGE
	 * 	-send length of message
	 * 	-send message
	 */
	BANG_DEBUG_REQUEST,
	/**
	 * do:
	 * 	-send BANG_SEND_MODULE
	 * 	-send length of module
	 * 	-send module
	 * 	NOTE:  NOT THE struct, the actual file.
	 */
	BANG_SEND_MODULE_REQUEST,
	BANG_NUM_REQUESTS
} BANG_request_type;
#endif
