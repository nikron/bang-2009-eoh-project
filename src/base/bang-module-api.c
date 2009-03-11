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
#include"bang-utils.h"
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
 */
static void get_uuid_from_id(uuid_t uuid, int id, BANG_module_info *info);

/**
 * \param info The info about a module.
 *
 * \return A null terminated list of uuids.
 *
 * \brief Gets a list of valid uuid's from the information.
 */
static uuid_t* get_valid_routes(BANG_module_info *info);

static void get_uuid_from_id(uuid_t uuid, int id, BANG_module_info *info) {
	BANG_read_lock(info->lck);

	if (id < 0 || id > info->peers_info->peer_number ||
		info->peers_info->validity[id] == 0) {
		uuid_clear(uuid);
	} else {
		uuid_copy(uuid,info->peers_info->uuids[id]);
	}

	BANG_read_unlock(info->lck);
}

static uuid_t* get_valid_routes(BANG_module_info *info) {
	BANG_read_lock(info->lck);

	int i, size = 1;
	uuid_t *valid_routes = calloc(size,sizeof(uuid_t));
	for (i = 0; i < info->peers_info->peer_number; ++i) {
		if (info->peers_info->validity[i]) {
			valid_routes = realloc(valid_routes,size++ * sizeof(uuid_t));
			uuid_copy(valid_routes[i],info->peers_info->uuids[i]);
		}
	}
	uuid_clear(valid_routes[size]);

	BANG_read_unlock(info->lck);

	return valid_routes;
}

void BANG_debug_on_all_peers(BANG_module_info *info, char *message) {
	/* The first slot will be us, so skip that. */
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

	BANG_request *req = new_BANG_request(BANG_MODULE_PEER_REQUEST,NULL,0);
	/* TODO: Put the module info inside the data... */

	while ((cur = BANG_linked_list_pop(peers_to_bug)) != NULL) {
		BANG_request_peer_id(*cur,req);
		free(cur);
	}

	free_BANG_linked_list(peers_to_bug,NULL);
	free(valid_routes);
}

int BANG_number_of_active_peers(BANG_module_info *info) {
	/* The way we store ids may change in the future, so this is a simple
	 * wrapper function
	 */
	int i = 0,j = 0;

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
	fprintf(stderr,"Requesting a job from authority %d!\n",id);
#endif
	uuid_t auth, me;
	get_uuid_from_id(auth,id,info);
	get_uuid_from_id(me,info->my_id,info);

	if (!uuid_is_null(auth)) {
		BANG_route_request_job(auth,me);
	}

}

void BANG_finished_request(BANG_module_info *info, BANG_job *job) {
#ifdef BDEBUG_1
	fprintf(stderr,"Finished a job from authority %d!\n",job->authority);
#endif

	uuid_t auth,peer;
	get_uuid_from_id(auth,job->authority,info);
	get_uuid_from_id(peer,job->peer,info);

	if (!uuid_is_null(auth)) {
		BANG_route_finished_job(auth,peer,job);
	}
}

void BANG_send_job(BANG_module_info *info, BANG_job *job) {
	uuid_t route;
	get_uuid_from_id(route,job->peer,info);

	if (!uuid_is_null(route)) {
		BANG_route_job(info->peers_info->uuids[info->my_id],route,job);
	}
}
