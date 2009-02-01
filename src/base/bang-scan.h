#ifndef __BANG_SCAN_H
#define __BAN_SCAN_H

void BANG_listen();

int BANG_announce();

/**
 * \param to_accept Whether or not the library will try to
 * immediately connect to a peer.
 *
 * If true, the bang library will try to automatically try to
 * connect to any new peers.
 *
 * default: true
 */
void BANG_set_acceptance(char to_accept);
#endif
