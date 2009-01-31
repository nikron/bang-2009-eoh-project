#ifndef __BANG_PEER_THREADS
#define __BANG_PEER_THREADS
#include"bang-types.h"
#include<poll.h>
#include<semaphore.h>

/**
 * Holds requests of the peers in a linked list.
 */
typedef struct {
	/**
	 * Signals the thread that a new requests has been added.
	 */
	sem_t num_requests;
	/**
	 * A linked list of requests
	 */
	BANG_linked_list *requests;
} BANG_requests;

/**
 * The peers structure is a reader-writer structure.  That is, multiple threads
 * can be reading data from the structure, but only one process can change the
 * size at once.
 */
typedef struct {
	/**
	 * The unique id of a peer that the client library will use.
	 */
	int peer_id;
	/**
	 * The name supplied by the peer.
	 */
	char *peername;
	/**
	 * The actual socket of the peer.
	 */
	int socket;
	/**
	 * The thread that is reading in from the peer.
	 */
	pthread_t receive_thread;
	/**
	 * The list of requests that the send thread will execute.
	 */
	BANG_requests *requests;
	/**
	 * Performs the requests in the requests list.
	 */
	pthread_t send_thread;
	/**
	 * Poll set up so that the peer wont have to reconstruct it at every step
	 * note: perhaps pass this around rather than put it in global space.
	 */
	struct pollfd pfd;
} BANG_peer;

/**
 * \param self The peer to be requested.
 * \param request Request to be given to the peer.
 *
 * \brief Adds a request to a peer structure.
 */
void request_peer(BANG_peer *self, BANG_request *request);

/**
 * \param peer_id Id to be used for the peer.
 * \param socket Socket to be used for the peer.
 *
 * \brief Allocates and returns a new peer.
 */
BANG_peer* new_BANG_peer(int peer_id, int socket);

/**
 * \brief Removes and frees a peer and its resources.
 */
void free_BANG_peer(BANG_peer *p);

/**
 * \param p The peer to stop.
 *
 * \brief Stops a peer's thread.  Note:  This
 * will most likely make the remote end hang up.
 */
void BANG_peer_stop(BANG_peer *p);

/**
 * \param p The peer to start.
 *
 * Starts a peer's threads.
 */
void BANG_peer_start(BANG_peer *p);

/**
 * \param self_info Information about the peer.
 *
 * \brief A peer connection thread.
 */
void* BANG_read_peer_thread(void *self_info);

/**
 * \param self_info Information about the peer.
 *
 * \brief A peer connection thread.
 */
void* BANG_write_peer_thread(void *self_info);
#endif
