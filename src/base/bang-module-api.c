/**
 * \file bang-module-api.c
 * \author Nikhil Bysani
 * \date December 24, 2009
 *
 * \brief  Fufills an api request that a module may have.
 */

#include"bang-module-api.h"
#include"bang-com.h"
#include"bang-routing.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<uuid/uuid.h>

/**
 * \param id The id to extract.
 * \param info The place the uuid will be from.
 * \param uuid The place the uuid is placed.
 *
 * \brief Gets a uuid from the info, or puts NULL there
 * TODO: Use locks!
 */
static void get_uuid_from_id(uuid_t uuid, int id, BANG_module_info *info);

static uuid_t* get_valid_routes(BANG_module_info *info);

static void get_uuid_from_id(uuid_t uuid, int id, BANG_module_info *info) {
	if (id < 0 || id > info->peers_info->peer_number ||
		info->peers_info->validity[id] == 0) {
		uuid_clear(uuid);
	} else {
		uuid_copy(uuid,info->peers_info->uuids[id]);
	}
}

static uuid_t* get_valid_routes(BANG_module_info *info) {
	int i, size = 1;
	uuid_t *valid_routes = calloc(size,sizeof(uuid_t));
	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (info->peers_info->validity[i]) {
			valid_routes = realloc(valid_routes,size++ * sizeof(uuid_t));
			uuid_copy(valid_routes[i],info->peers_info->uuids[i]);
		}
	}
	uuid_clear(valid_routes[size]);

	return valid_routes;
}

/* TODO: Implement this.  Well, this is basically what are our app is all about. 
 * TODO: LOCKS
 *
 * Note: This is no longer what out app is about, I added another layer of indirection.
 */
void BANG_debug_on_all_peers(BANG_module_info *info, char *message) {
	/* The first slot will be us, so skip that. */
	fprintf(stderr,"%s",message);

	int i = 1;
	uuid_t *valid_routes = get_valid_routes(info);

	for (; !uuid_is_null(valid_routes[i]); ++i) {
		BANG_route_send_message(valid_routes[i],message);
	}
}

void BANG_get_me_peers(BANG_module_info *info) {
	/* This does not use routing...! */
	uuid_t *valid_routes = get_valid_routes(info);
	unsigned int length = 0;
	while (!uuid_is_null(valid_routes[length])) {
			++length;
	}

	/*int **peers_to_bug =*/ BANG_not_route_get_peer_id(valid_routes,length);

	free(valid_routes);
}

int BANG_number_of_active_peers(BANG_module_info *info) {
	/* The way we store ids may change in the future, so this is a simple
	 * wrapper function 
	 *
	 * TODO: Locks!
	 */
	return info->peers_info->peer_number;
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
	get_uuid_from_id(authority,id,info);

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
	get_uuid_from_id(auth,authority,info);
	get_uuid_from_id(route,peer,info);

	if (!uuid_is_null(auth) && !uuid_is_null(route)) {
		BANG_route_assertion_of_authority(auth,route);
	}
}

void BANG_request_job(BANG_module_info *info, int id) {
#ifdef BDEBUG_1
	fprintf(stderr,"Requesting a job from authority %d with blocking at %d!\n",id,blocking);
#endif
	uuid_t route;
	get_uuid_from_id(route,id,info);

	if (!uuid_is_null(route)) {
		BANG_route_request_job(route);
	}

}

void BANG_finished_request(BANG_module_info *info, BANG_job *job) {
	uuid_t route;
	get_uuid_from_id(route,job->authority,info);

	if (!uuid_is_null(route)) {
		BANG_route_finished_job(route,job);
	}
}

void BANG_send_job(BANG_module_info *info, BANG_job *job) {
	uuid_t route;
	get_uuid_from_id(route,job->peer,info);

	if (!uuid_is_null(route)) {
		BANG_route_job(info->peers_info->uuids[info->my_id],route,job);
	}
}
