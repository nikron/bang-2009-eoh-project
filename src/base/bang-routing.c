#include"bang-com.h"
#include"bang-routing.h"
#include"bang-module.h"
#include"bang-module-api.h"
#include"bang-signals.h"
#include"bang-types.h"
#include"bang-utils.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<uuid/uuid.h>

enum remote_route {
	REMOTE,
	LOCAL
};

typedef struct {
	BANG_linked_list *not_peers_list;
	uuid_t *routes;
} not_peers_struct;

typedef struct {
	int peer_id;
	char *module_name;
	unsigned char *module_version;
} peer_route;

typedef struct {
	enum remote_route remote;
	peer_route *pr;
	BANG_module *mr;
} peer_or_module;

typedef struct {
	int peer_id;
	uuid_t route;
} peer_uuid_pair;

typedef struct {
	int peer_id;
	BANG_linked_list *routes;
	BANG_rw_syncro *lck;
} peer_to_uuids;

static void create_peer_to_uuids(int signal, int num_ps, void **ps);
static void remove_peer_to_uuids(int signal, int num_ps, void **ps);
static int add_uuid_to_peer(const void *p, void *peer_uuid);
static int remove_uuid_from_peer(const void *p, void *r);
static void free_peer_to_uuids(void *old);
static peer_to_uuids* new_peer_to_uuids(int peer_id);
static int uuid_hashcode(const void *uuid);
static int uuid_ptr_compare(const void *uuid1, const void *uuid2);
static peer_or_module *new_pom_module_route(BANG_module *module);
static peer_or_module *new_pom_peer_route(int peer_id, char *module_name, unsigned char *module_version);
static BANG_request* create_request(enum BANG_request_types request, uuid_t authority, uuid_t peer);
static BANG_request* create_request_with_message(enum BANG_request_types request, char *message);
static BANG_request* create_request_with_job(enum BANG_request_types request, uuid_t authority, uuid_t peer, BANG_job *job);
static void find_not_peers(const void *info_about_peers, void *store_not_peers);

static BANG_rw_syncro *routes_lock = NULL;
static BANG_hashmap *routes = NULL;
static BANG_linked_list *peers_list = NULL;

static void create_peer_to_uuids(int signal, int num_ps, void **peer_ids) {
	int i;
	int *peer_id;

	if (signal == BANG_PEER_ADDED) {
		for (i = 0; i < num_ps; ++i) {
			peer_ids = peer_ids[i];

			BANG_write_lock(routes_lock);

			BANG_linked_list_push(peers_list, new_peer_to_uuids(*peer_id));

			BANG_write_unlock(routes_lock);
		}
	}

	for (i = 0; i < num_ps; ++i) {
		free(peer_ids[i]);
	}
	free(peer_ids);
}

static void remove_peer_to_uuids(int signal, int num_ps, void **peer_ids) {
	int i;
	int *peer_id;

	if (signal == BANG_PEER_REMOVED) {
		BANG_linked_list *temp = new_BANG_linked_list();
		peer_to_uuids *cur;
		/* TODO: DO SOMETHING! */


		BANG_write_lock(routes_lock);

		while ((cur = BANG_linked_list_pop(peers_list)) != NULL) {
			for (i = 0; i < num_ps; ++i) {
				peer_id = peer_ids[i];

				if (*peer_id == cur->peer_id) {
					free_peer_to_uuids(cur);
				} else {
					BANG_linked_list_push(temp,cur);
				}
			}
		}

		free_BANG_linked_list(peers_list,NULL);
		peers_list = temp;

		BANG_write_unlock(routes_lock);
	}

	for (i = 0; i < num_ps; ++i) {
		free(peer_ids[i]);
	}
	free(peer_ids);
}

static int add_uuid_to_peer(const void *p, void *peer_uuid) {
	peer_to_uuids *ptu = (void*)p;
	peer_uuid_pair *pup = peer_uuid;

	if (pup->peer_id == ptu->peer_id) {
		BANG_write_lock(ptu->lck);

		BANG_linked_list_push(ptu->routes,&pup->route);

		BANG_write_unlock(ptu->lck);

		return 0;
	}

	return 1;
}

static int remove_uuid_from_peer(const void *p, void *r) {
	peer_to_uuids *ptu = (void*)p;
	uuid_t route;
	uuid_copy(route,*((uuid_t*)r));
	int ret = 1;
	uuid_t *cur_route;
	BANG_linked_list *new = new_BANG_linked_list();

	BANG_write_lock(ptu->lck);

	while ((cur_route = BANG_linked_list_pop(ptu->routes)) != NULL) {
		if (uuid_compare(*cur_route,route) != 0) {
			BANG_linked_list_push(new, cur_route);
			ret = 0;

		}
	}

	free_BANG_linked_list(ptu->routes,NULL);
	ptu->routes = new;

	BANG_write_unlock(ptu->lck);

	return ret;
}

static void free_peer_to_uuids(void *old) {
	peer_to_uuids *dead = old;

	BANG_write_lock(dead->lck);
	free_BANG_linked_list(dead->routes, NULL);
	BANG_write_unlock(dead->lck);

	free_BANG_rw_syncro(dead->lck);
	free(dead);
}

static peer_to_uuids* new_peer_to_uuids(int peer_id) {
	peer_to_uuids *new = malloc(sizeof(peer_to_uuids));

	new->peer_id = peer_id;
	new->routes = new_BANG_linked_list();
	new->lck = new_BANG_rw_syncro();

	return new;
}

static int uuid_hashcode(const void *uuid) {
	assert(uuid != NULL);

	int hc = (*((uuid_t*)uuid)[0] << 2) & (*((uuid_t*)uuid)[15]);

	return hc;
}

static int uuid_ptr_compare(const void *uuid1, const void *uuid2) {
	return uuid_compare(*((uuid_t*)uuid1),*((uuid_t*)uuid2));
}

static peer_or_module *new_pom_module_route(BANG_module *module) {
	peer_or_module *new = malloc(sizeof(peer_or_module));

	new->remote = LOCAL;

	new->mr = module;

	return new;
}

static peer_or_module *new_pom_peer_route(int peer_id, char *module_name, unsigned char *module_version) {
	peer_or_module *new = malloc(sizeof(peer_or_module));

	new->remote = REMOTE;

	new->pr = malloc(sizeof(peer_route));

	new->pr->peer_id = peer_id;
	new->pr->module_name = module_name;
	new->pr->module_version = module_version;

	return new;
}

static BANG_request* create_request(enum BANG_request_types request, uuid_t authority, uuid_t peer) {
	int request_length = sizeof(uuid_t) * 2;
	void *request_data = malloc(request_length);

	memcpy(request_data,authority,sizeof(uuid_t));
	memcpy(request_data + sizeof(uuid_t),peer,sizeof(uuid_t));

	return new_BANG_request(request,request_data,request_length);
}

static BANG_request* create_request_with_message(enum BANG_request_types request, char *message) {
	return new_BANG_request(request,message,strlen(message));
}

static BANG_request* create_request_with_job(enum BANG_request_types request, uuid_t authority, uuid_t peer, BANG_job *job) {
	int metadata_length = sizeof(uuid_t) * 2 + 4 + LENGTH_OF_LENGTHS;
	int request_length = job->length + metadata_length;
	void *request_data = malloc(request_length);

	memcpy(request_data, authority, sizeof(uuid_t));
	memcpy(request_data + sizeof(uuid_t), peer, sizeof(uuid_t));
	memcpy(request_data + sizeof(uuid_t) * 2, &job->job_number, 4);
	memcpy(request_data + sizeof(uuid_t) * 2 + 4, &job->length, LENGTH_OF_LENGTHS);
	memcpy(request_data + metadata_length, &job->data, job->length);

	return new_BANG_request(request,request_data,request_length);
}

void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	/* assert(routes != NULL); */
	assert(job != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	BANG_read_unlock(routes_lock);

	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job(route->mr,job,authority,peer);

	} else {

		BANG_request *request = create_request_with_job(BANG_SEND_JOB_REQUEST,authority,peer,job);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_job_to_uuids(uuid_t authority, uuid_t *peers, BANG_job *job) {
	/* assert(routes != NULL); */
	assert(job != NULL);

	int i;

	for (i = 0; !uuid_is_null(peers[i]); ++i) {
		BANG_route_job(authority,peers[i],job);
	}
}

void BANG_route_finished_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	/* assert(routes != NULL); */
	assert(job != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&authority);
	BANG_read_unlock(routes_lock);

	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_finished(route->mr,job,authority,peer);

	} else {

		BANG_request *request = create_request_with_job(BANG_SEND_FINISHED_JOB_REQUEST,authority,peer,job);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_request_job(uuid_t peer, uuid_t authority) {
	/* assert(routes != NULL); */
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&authority);
	BANG_read_unlock(routes_lock);

	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_request(route->mr,authority,peer);

	} else {

		BANG_request *request = create_request(BANG_SEND_REQUEST_JOB_REQUEST,authority,peer);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer) {
	/* assert(routes != NULL); */
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	BANG_read_unlock(routes_lock);

	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_available(route->mr,authority,peer);

	} else {

		BANG_request *request = create_request(BANG_SEND_AVAILABLE_JOB_REQUEST,authority,peer);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_send_message(uuid_t peer, char *message) {
	/* assert(routes != NULL); */
	assert(message != NULL);
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	BANG_read_unlock(routes_lock);

	if (route == NULL) return;

	if (route->remote == LOCAL) {
		fprintf(stderr,"%s",message);

	} else {

		BANG_request *request = create_request_with_message(BANG_DEBUG_REQUEST,message);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

int BANG_route_get_peer_id(uuid_t peer) {
	/* assert(routes != NULL); */
	assert(!uuid_is_null(peer));

	BANG_read_lock(routes_lock);
	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	BANG_read_unlock(routes_lock);

	if (route->remote == LOCAL) {
		return -1;

	} else {

		return route->pr->peer_id;
	}
}

static int check_route_match(const void *route_of_peer, void *routes_to_find) {
	uuid_t *route_to_check = (void*) route_of_peer;
	uuid_t *routes_check_against = routes_to_find;
	int i;

	for (i = 0; !uuid_is_null(routes_check_against[i]); ++i) {
		if (uuid_compare(*route_to_check,routes_check_against[i]) == 0)
			return 0;
	}

	return 1;
}

static void find_not_peers(const void *info_about_peers, void *store_not_peers) {
	peer_to_uuids *ptu = (void*) info_about_peers;
	not_peers_struct *not_peers = store_not_peers;
	int *found_peer_id;

	BANG_read_lock(ptu->lck);

	if (BANG_linked_list_conditional_iterate(ptu->routes,&check_route_match,not_peers->routes)) {
		found_peer_id = malloc(sizeof(int));
		*found_peer_id = ptu->peer_id;
		BANG_linked_list_push(not_peers->not_peers_list,found_peer_id);
	}

	BANG_read_unlock(ptu->lck);
}

/* TODO: THIS IS IMPORTANT! */
BANG_linked_list* BANG_not_route_get_peer_id(uuid_t *peers) {
	assert(peers != NULL);

	not_peers_struct not_peers;
	not_peers.not_peers_list = new_BANG_linked_list();
	not_peers.routes = peers;

	BANG_read_lock(routes_lock);

	BANG_linked_list_iterate(peers_list,&find_not_peers,&not_peers);

	BANG_read_lock(routes_lock);

	return not_peers.not_peers_list;
}

/* TODO: THIS IS IMPORTANT! */
void BANG_route_new_peer(uuid_t peer, uuid_t new_peer) {
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(new_peer));
}

/* TODO: THIS IS IMPORTANT! */
void BANG_route_remove_peer(uuid_t peer, uuid_t old_peer) {
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(old_peer));
}

void BANG_register_module_route(BANG_module *module) {
	assert(module != NULL);

	uuid_t new_uuid;
	uuid_generate(new_uuid);

	peer_or_module *pom = new_pom_module_route(module);

	BANG_write_lock(routes_lock);

	BANG_hashmap_set(routes,&new_uuid,pom);

	BANG_write_unlock(routes_lock);
}

void BANG_register_peer_route(uuid_t uuid, int peer_id, char *module_name, unsigned char* module_version) {
	assert(!uuid_is_null(uuid));
	assert(peer_id != -1);

	peer_or_module *pom = new_pom_peer_route(peer_id,module_name,module_version);

	peer_uuid_pair pup;
	pup.peer_id = peer_id = peer_id;
	uuid_copy(pup.route,uuid);

	BANG_write_lock(routes_lock);

	BANG_hashmap_set(routes,&uuid,pom);

	if (BANG_linked_list_conditional_iterate(peers_list,&add_uuid_to_peer,&pup)) {
		/* ERROR: maybe append a peer_to_uuids? */
	}

	BANG_write_unlock(routes_lock);
}

void BANG_deregister_route(uuid_t route) {
	assert(!uuid_is_null(route));

	BANG_write_lock(routes_lock);

	BANG_hashmap_set(routes,&route,NULL);
	BANG_linked_list_conditional_iterate(peers_list,&remove_uuid_from_peer,&route);

	BANG_write_unlock(routes_lock);
}

void BANG_route_init() {
	routes_lock = new_BANG_rw_syncro();

	BANG_write_lock(routes_lock);

	routes = new_BANG_hashmap(&uuid_hashcode,&uuid_ptr_compare);
	peers_list = new_BANG_linked_list();

	BANG_write_unlock(routes_lock);

	BANG_install_sighandler(BANG_PEER_ADDED, &create_peer_to_uuids);
	BANG_install_sighandler(BANG_PEER_REMOVED, &remove_peer_to_uuids);
}

void BANG_route_close() {
	BANG_write_lock(routes_lock);

	free_BANG_hashmap(routes);
	free_BANG_linked_list(peers_list,&free_peer_to_uuids);

	BANG_write_unlock(routes_lock);

	free_BANG_rw_syncro(routes_lock);

	routes = NULL;
	peers_list = NULL;
	routes_lock = NULL;
}
