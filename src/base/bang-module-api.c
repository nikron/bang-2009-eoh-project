/**
 * \file bang-module-api.c
 * \author Nikhil Bysani
 * \date December 24, 2009
 *
 * \brief  Fufills an api request that a module may have.
 */

#include<stdio.h>
#include"bang-module-api.h"

/* TODO: Implement this.  Well, this is basically what are our app is all about. */
void BANG_debug_on_all_peers(char *message) {
	fprintf(stderr,message);
}

void BANG_get_me_peers() {
}

int BANG_number_of_active_peers() {
	return 1;
}

int BANG_get_my_id() {
	return 0;
}

void BANG_assert_authority(int id) {
	fprintf(stderr,"%d is asserting authority!\n",id);
}

void BANG_assert_authority_to_peer(int authority, int peer) {
	fprintf(stderr,"%d is asserting authority to %d!\n",authority,peer);
}

BANG_job* BANG_request_job(int id, char blocking) {
	fprintf(stderr,"Requesting a job from authority %d with blocking at %d!\n",id,blocking);
	return NULL;
}

void BANG_finished_request(BANG_job *job) {
	fprintf(stderr,"Finished job for authority %d!\n",job->authority);
}

void BANG_send_job( BANG_job *job) {
	fprintf(stderr,"Sending job to %d by authority %d!\n",job->peer,job->authority);
}
