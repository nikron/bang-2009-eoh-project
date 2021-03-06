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
#include<stdint.h>

/**
 * The major version of the library.
 * {0}.1.1
 */
#define BANG_MAJOR_VERSION 0
/**
 * The middle version of the library.
 * 0.{1}.1
 */
#define BANG_MIDDLE_VERSION 1
/**
 * The minor version of the library.
 * 0.1.{1}
 */
#define BANG_MINOR_VERSION 1

static const unsigned char BANG_LIBRARY_VERSION[3] = {
	BANG_MAJOR_VERSION,
	BANG_MIDDLE_VERSION,
	BANG_MINOR_VERSION };

/**
 * The structure that represents a module for the program.
 */
typedef struct {
	/**
	 * Information about the module.
	 * such as name, and its peers.
	 */
	BANG_module_info *info;
	/**
	 * Callbacks to interact with the module.
	 */
	BANG_callbacks *callbacks;
	/**
	 * The module initialize method.
	 */
	BANG_callbacks* (*module_init)(BANG_api*);
	/**
	 * The module run method.
	 */
	void (*module_run)(BANG_module_info*);

	/**
	 * The hash of the module.
	 */
	unsigned char *md;

	/**
	 * The actual handle of the module.
	 */
	void *handle;

	/**
	 * The path to the module.
	 */
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
typedef void (*BANGSignalHandler)(int,int,void**);

/**
 * The signals that can be sent by library.
 * TODO: Detail all the signals, especially their arguments.
 */
enum BANG_signals {
	/**
	 * The bang server has successfully bound.
	 */
	BANG_BIND_SUC = 0,
	/**
	 * A peer has been added and is now in active status.
	 *
	 * arguement:
	 * int number of ids
	 * **int (null terminated peer ids)
	 */
	BANG_PEER_ADDED,
	/**
	 * A peer has been removed.
	 */
	BANG_PEER_REMOVED,
	/**
	 * A module is going to be run.
	 * arg: the BANG_module being run.
	 */
	BANG_RUNNING_MODULE,
	/**
	 * Client sends a module to peer.
	 * arg: module_name | '\0' | module version (3 bytes) | peer_id (int)
	 */
	BANG_MODULE_REQUEST,
	/**
	 * Send request to see if peer has module.
	 * arg: module_name | '\0' | module version (3 bytes) | peer_id (int)
	 */
	BANG_MODULE_EXISTS,
	/**
	 * A job has been requested.
	 */
	BANG_JOB_REQUEST,
	/**
	 * A job has been finished.
	 */
	BANG_JOB_FINISHED,
	/**
	 * A new job is available.
	 */
	BANG_JOB_AVAILABLE,
	/**
	 * The bang server has started.
	 * arg: the server socket.
	 */
	BANG_SERVER_STARTED,
	/**
	 * The bang server has stopped.
	 */
	BANG_SERVER_STOPPED,
	/**
	 * The server's bind has failed.
	 */
	BANG_BIND_FAIL,
	/**
	 * The server could not get addrinfo.
	 */
	BANG_GADDRINFO_FAIL,
	/**
	 * The thread could not connect.
	 */
	BANG_CONNECT_FAIL,
	/**
	 * The server could not listen.
	 */
	BANG_LISTEN_FAIL,
	/**
	 * There was an error loading the module.
	 * arg: the output of dlerror
	 */
	BANG_MODULE_ERROR,
	/**
	 * A peer has connected and is being set up.
	 */
	BANG_PEER_CONNECTED,
	/**
	 * Went a peer has recieved a new module.
	 * args: a binary that has to be written to disk ~
	 * careful with this..
	 */
	BANG_RECEIVED_MODULE,
	/**
	 * A peer has disconnected and is being removed.
	 * args:t5
	 */
	BANG_PEER_DISCONNECTED,
	/**
	 * A peer has responded to the broadcast.
	 * args:peer's address
	 */
	BANG_PEER_WANTS_TO_CONNECT,
	/**
	 * Request all the peers a request.
	 */
	BANG_REQUEST_ALL,
	/**
	 * Don't know if we can (will) implement this signals
	 */
	BANG_CLOSE_ALL,
	/**
	 * The number of signals.  All new signals should be above this line.
	 */
	BANG_NUM_SIGS
};

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
 * peer1: BANG_BYE
 *
 */

/**
 * Length of the headers.
 */
typedef uint32_t BANG_header;
#define LENGTH_OF_HEADER sizeof(BANG_header)

/**
 * Length of any lengths sent.
 */
#define LENGTH_OF_LENGTHS 4
/**
 * Length of any versions sent.
 */
#define LENGTH_OF_VERSION 3

enum BANG_headers {
	/**
	 * message:
	 *	-BANG_HELLO 		(4 unsigned bytes)
	 *	-BANG_VERSION 		(8 bytes to a double)
	 *	-length of name 	(4 unsigned bytes)
	 *	-peer name 		(char*)
	 */
	BANG_HELLO = 0,

	/**
	 * ->BANG_DEBUG_REQUEST (corresponding request type)
	 * sends a debug message that should be printed out on the remote
	 * end.
	 * message:
	 *	-BANG_DEBUG_MESSAGE 	(4 unsigned bytes)
	 *	-length of message 	(4 unsigned bytes)
	 *	-message 		(char*)
	 */
	BANG_DEBUG_MESSAGE,

	/**
	 * ->BANG_SEND_MODULE_REQUEST (corresponding request type)
	 * sends a module.  very rude to do this if you haven't
	 * asked if they want it it, or if they have requested it.
	 *
	 * message:
	 *	-BANG_SEND_MODULE 	(4 unsigned bytes)
	 *	-length of module 	(4 unsigned bytes)
	 *	-module 		(void*)
	 * NOTE: The file, not the struct.
	 */
	BANG_SEND_MODULE,

	/**
	 * -> (corresponding request type)
	 * Requests a module from the remote end
	 * message:
	 *	-BANG_REQUEST_MODULE	(4 unsigned bytes)
	 *	-length of name		(4 unsigned bytes)
	 *	-module_name		(char*)
	 *	-'\0'			(1 byte)
	 *	-version		(3 unsigned bytes)
	 */
	BANG_REQUEST_MODULE,

	/**
	 * ->BANG_MODULE_PEER_REQUEST (corresponding request type)=
	 * Asks the remote end if they want a module, if yes
	 * they'll respond with a request
	 * message:
	 *	-BANG_EXISTS_MODULE 	(4 unsigned bytes)
	 *	-length of name		(4 unsigned bytes)
	 *	-module_name 		(char*)
	 *	-'\0'			(1 byte)
	 *	-version		(3 unsigned bytes)
	 */
	BANG_EXISTS_MODULE,

	/**
	 * ->BANG_SEND_JOB_REQUEST (corresponding request type)
	 * Send a job to a uuid.  The read thread passes the data
	 * off to a router after constructing a job.
	 * message:
	 *	-BANG_SEND_JOB		(4 unsigned bytes)
	 *	-Authority		(16 bytes)
	 *	-Peer			(16 bytes)
	 *	-job_numer		(4 bytes)
	 *	-job_length		(4 unsigned bytes)
	 *	-job_data		(job_length bytes)
	 */
	BANG_SEND_JOB,

	/**
	 * ->BANG_SEND_REQUEST_JOB_REQUEST (corresponding request type)
	 * Requests a job from a uuid.
	 * message:
	 *	-BANG_REQUEST_JOB 	(4 unsigned bytes)
	 *	-Authority		(16 bytes)
	 *	-Peer			(16 bytes)
	 */
	BANG_REQUEST_JOB,

	/**
	 * -> (corresponding request type)
	 * message:
	 * 	-BANG_FINISHED_JOB
	 */
	BANG_FINISHED_JOB,

	/**
	 * ->BANG_SEND_AVAILABLE_JOB_REQUEST (corresponding request type)
	 * message:
	 * 	-BANG_AVAIlABLE_JOB
	 * 	-authority		(16 bytes)
	 * 	-peer			(16 bytes)
	 */
	BANG_AVAILABLE_JOB,

	/**
	 * tells the remote end that version is wrong, and that
	 * they are hanging up
	 * message:
	 *	-BANG_MISMATCH_VERSION	(4 unsigned bytes)
	 *	-our version 		(3 unsigned bytes)
	 */
	BANG_MISMATCH_VERSION,

	/**
	 * message:
	 *	-BANG_BYE 		(4 unsigned bytes)
	 *
	 * TODO: This will most defintely have more things involved.
	 */
	BANG_BYE,

	/**
	 * Number of headers.*/
	BANG_NUM_HEADERS
};

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
	 * <-BANG_DEBUG_MESSAGE (corresponding header type)
	 * BANG_request.request:
	 * | char* | '\0'|
	 * BANG_request.length:
	 * strlen(char*) + 1
	 *
	 * do:
	 *	-send BANG_DEBUG_MESSAGE
	 *	-send length of message
	 *	-send message
	 */
	BANG_DEBUG_REQUEST,

	/**
	 * <-BANG_EXISTS_MODULE (corresponding header type)
	 * BANG_request.type == BANG_MODULE_PEER_REQUEST
	 * BANG_request.request:
	 * |  module_name | '\0' | 3 bytes module version|
	 * BANG_request.length == length of name + 3 bytes
	 * do:
	 *
	 */
	BANG_MODULE_PEER_REQUEST,

	/**
	 * <-BANG_SEND_JOB (corresponding header type)
	 * BANG_request.type == BANG_SEND_JOB_REQUEST
	 * BANG_request.request:
	 * | auth | peer | job_number | job_length | job data |
	 *
	 * do:
	 *	-send out everything appended to a header.
	 *
	 */
	BANG_SEND_JOB_REQUEST,

	BANG_SEND_FINISHED_JOB_REQUEST,

	/**
	 * <-BANG_REQUEST_JOB (corresponding header type)
	 * BANG_request.type == BANG_SEND_REQUEST_JOB_REQUEST
	 */
	BANG_SEND_REQUEST_JOB_REQUEST,

	/**
	 * <-BANG_AVAILABLE_JOB (corresponding header type)
	 *
	 */
	BANG_SEND_AVAILABLE_JOB_REQUEST,

	/**
	 * <-BANG_SEND_MODULE (corresponding header type)
	 * do:
	 *	-send BANG_SEND_MODULE
	 *	-send length of module
	 *	-send module
	 *	NOTE:  NOT THE struct, the actual file.
	 */
	BANG_SEND_MODULE_REQUEST,

	BANG_NUM_REQUESTS
};

/**
 * A request to be made to a write peer thread.
 */
typedef struct {
	/**
	 * The type of request.
	 */
	enum BANG_request_types type;
	/**
	 * Information part of the request.
	 */
	void *request;
	/**
	 * Length of the information.
	 */
	unsigned int length;
} BANG_request;

BANG_request* new_BANG_request(int type, void *data, int length);
void free_BANG_request(void *req);

/**
 * When reading from a file, you should do it incremently at 1k bytes at a time!
 */
#define UPDATE_SIZE 1024
#endif
