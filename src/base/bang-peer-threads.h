#ifndef __BANG_PEER_THREADS
#define __BANG_PEER_THREADS
#include"bang-types.h"
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
