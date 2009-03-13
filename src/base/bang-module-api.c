/**
 * \file bang-module-api.c
 * \author Nikhil Bysani
 * \date December 24, 2009
 *
 * \brief  Fufills an api request that a module may have.
 */

#include"bang-module-api.h"
#include"bang-module.h"
#include"bang-com.h"
#include"bang-routing.h"
#include"bang-utils.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<uuid/uuid.h>


/**
 * \param info The info about a module.
 *
 * \return A null terminated list of uuids.
 *
 * \brief Gets a list of valid uuid's from the information.
 */
static uuid_t* get_valid_routes(BANG_module_info *info);

static uuid_t* get_valid_routes(BANG_module_info *info) {
	BANG_read_lock(info->lck);

	int i, size = 1;
	uuid_t *valid_routes = calloc(size,sizeof(uuid_t));

#ifdef BDEBUG_1
	fprintf(stderr,"Peer info for %s is %d.\n",info->module_name, info->peers_info->peer_number);
#endif

	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (info->peers_info->validity[i]) {
			size++;
			valid_routes = realloc(valid_routes, size * sizeof(uuid_t));
			uuid_copy(valid_routes[size - 2],info->peers_info->uuids[i]);
		}
	}

	uuid_clear(valid_routes[size - 1]);

#ifdef BDEBUG_1
	fprintf(stderr,"Number of valid routes for %s is %d.\n",info->module_name, size);
#endif

	BANG_read_unlock(info->lck);

	return valid_routes;
}

void BANG_debug_on_all_peers(BANG_module_info *info, char *message) {
	/* The first slot will always be us, so skip that. */
	fprintf(stderr,"%s",message);

	int i = 1;
	uuid_t *valid_routes = get_valid_routes(info);

	for (; !uuid_is_null(valid_routes[i]); ++i) {
		BANG_route_send_message(valid_routes[i],message);
	}

}

/* TODO: Request something... */
void BANG_get_me_peers(BANG_module_info *info) {
	/* This does not use routing...! */
	uuid_t *valid_routes = get_valid_routes(info);

	BANG_linked_list *peers_to_bug = BANG_not_route_get_peer_id(valid_routes);
	int *cur;
	void *data;
	int data_length = info->module_name_length + 4 + sizeof(uuid_t);
	BANG_request *req;


	/* TODO: Put the module info inside the data... */

	while ((cur = BANG_linked_list_pop(peers_to_bug)) != NULL) {
		data = malloc(data_length);

		strcpy(data,info->module_name);
		memcpy(data + info->module_name_length + 1, info->module_version, 3);
		memcpy(data + info->module_name_length + 4,  info->peers_info->uuids[info->my_id], sizeof(uuid_t));

		req = new_BANG_request(BANG_MODULE_PEER_REQUEST,data,0);
		BANG_request_peer_id(*cur,req);
		free(cur);
	}

	free_BANG_linked_list(peers_to_bug,NULL);
	free(valid_routes);
}

int BANG_number_of_active_peers(BANG_module_info *info) {
	/*
	 * The way we store ids may change in the future, so this is a simple
	 * wrapper function
	 */
	int i = 0, j = 0;

	BANG_read_lock(info->lck);

	for (; i < info->peers_info->peer_number; ++i) {
		if (info->peers_info->validity[i])
			++j;
	}

	BANG_read_lock(info->lck);

	return j;
}

int BANG_get_my_id(BANG_module_info *info) {
	/* The way we store ids may change in the future, so this is a simple
	 * wrapper function */
	return info->my_id;
}

void BANG_assert_authority(BANG_module_info *info, int id) {
#ifdef BDEBUG_1
	fprintf(stderr,"%d is asserting authority!\n",id);
#endif
	uuid_t authority;
	BANG_get_uuid_from_local_id(authority,id,info);

	if (!uuid_is_null(authority)) {
		int i = 0;
		uuid_t *valid_routes = get_valid_routes(info);
		for (; !uuid_is_null(valid_routes[i]); ++i) {
			BANG_route_assertion_of_authority(authority,valid_routes[i]);
		}
	}
}

void BANG_assert_authority_to_peer(BANG_module_info *info, int authority, int peer) {
#ifdef BDEBUG_1
	fprintf(stderr,"%d is asserting authority to %d!\n",authority,peer);
#endif
	uuid_t auth,route;
	BANG_get_uuid_from_local_id(auth,authority,info);
	BANG_get_uuid_from_local_id(route,peer,info);

	if (!uuid_is_null(auth) && !uuid_is_null(route)) {
		BANG_route_assertion_of_authority(auth,route);
	}
}

void BANG_request_job(BANG_module_info *info, int id) {
#ifdef BDEBUG_1
	fprintf(stderr,"Requesting a job from authority %d!\n",id);
#endif
	uuid_t auth, me;
	BANG_get_uuid_from_local_id(auth,id,info);
	BANG_get_uuid_from_local_id(me,info->my_id,info);

	if (!uuid_is_null(auth)) {
		BANG_route_request_job(auth,me);
	}

}

void BANG_finished_request(BANG_module_info *info, BANG_job *job) {
#ifdef BDEBUG_1
	fprintf(stderr,"Finished a job from authority %d!\n",job->authority);
#endif

	uuid_t auth, peer;
	BANG_get_uuid_from_local_id(auth,job->authority,info);
	BANG_get_uuid_from_local_id(peer,job->peer,info);

	if (!uuid_is_null(auth)) {
		BANG_route_finished_job(auth,peer,job);
	}
}

void BANG_send_job(BANG_module_info *info, BANG_job *job) {
	uuid_t route;
	BANG_get_uuid_from_local_id(route, job->peer, info);

	if (!uuid_is_null(route)) {
		BANG_route_job(info->peers_info->uuids[info->my_id],route,job);
	}
}
