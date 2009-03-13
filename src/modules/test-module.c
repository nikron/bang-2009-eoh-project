/**
 * \file test-module.c
 * \author Nikhil Bysani
 * \date December 24, 2008
 *
 * \brief A simple test module to see if module functions are working
 * in order to flesh out a module api.
 */
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"../base/bang-module-api.h"

char BANG_module_name[5] = "test";
unsigned char BANG_module_version[] = {0,0,1};
BANG_api *api;

static BANG_job* new_test_job(int auth, int peer) {
	BANG_job *new = malloc(sizeof(BANG_job));

	new->data =  malloc(5);
	strcpy(new->data,"test");
	new->length = 5;
	new->job_number = 123;
	new->peer = peer;
	new->authority = auth;

	return new;
}

void test_job_available(BANG_module_info *info, int peer) {
	api->BANG_request_job(info, peer);
}
void test_job_done(BANG_module_info *info, BANG_job *job) {
	fprintf(stderr,"job %s\n", (char*) job->data);
	fprintf(stderr,"job number %d\n", job->job_number);

	int reassert = job->peer;
	api->BANG_assert_authority(info, reassert);
}

void test_outgoing_jobs(BANG_module_info *info, int peer) {
	fprintf(stderr,"sending out a job.\n");

	BANG_job *job = new_test_job(api->BANG_get_my_id(info), peer);
	api->BANG_send_job(info, job);
}

void test_incoming_job(BANG_module_info *info, BANG_job *job) {
	fprintf(stderr,"job %s\n", (char*) job->data);
	fprintf(stderr,"job number %d\n", job->job_number);

	api->BANG_finished_request(info, job);
}

void test_peer_added(BANG_module_info *info, int peer) {
	fprintf(stderr,"peer added %d\n,",peer);
	api->BANG_assert_authority(info, peer);
}

void test_peer_removed(BANG_module_info* info, int peer) {
	info = NULL;
	fprintf(stderr,"peer removed %d\n,",peer);
}

BANG_callbacks* BANG_module_init(BANG_api *get_api) {
	api = get_api;
	fprintf(stderr,"TEST:\t Module with name %s is initializing with version %d.%d.%d.\n",
			BANG_module_name,
			BANG_module_version[0],
			BANG_module_version[1],
			BANG_module_version[2]);
	BANG_callbacks *callbacks = malloc(sizeof(BANG_callbacks));

	callbacks->jobs_available = &test_job_available;
	callbacks->job_done = &test_job_done;
	callbacks->outgoing_jobs = &test_outgoing_jobs;
	callbacks->incoming_job = &test_incoming_job;
	callbacks->peer_added = &test_peer_added;
	callbacks->peer_removed = &test_peer_removed;

	return callbacks;
}

void BANG_module_run(BANG_module_info *info) {
	fprintf(stderr,"TEST:\t Module with name %s is running with version %d.%d.%d.\n",
			BANG_module_name,
			BANG_module_version[0],
			BANG_module_version[1],
			BANG_module_version[2]);
	api->BANG_debug_on_all_peers(info,"TESTING ON ALL PEERS!\n");
}
