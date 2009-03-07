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
	int peer_id;
	char *module_name;
	unsigned char *module_version;
} peer_route;

typedef struct {
	enum remote_route remote;
	peer_route *pr;
	BANG_module *mr;
} peer_or_module;

static peer_or_module *new_pom_peer_route(int peer_id, char *module_name, unsigned char *module_version);
static BANG_request* create_request(enum BANG_request_types request, uuid_t authority, uuid_t peer);
static BANG_request* create_request_with_message(enum BANG_request_types request, char *message);
static BANG_request* create_request_with_job(enum BANG_request_types request, uuid_t authority, uuid_t peer, BANG_job *job);

static BANG_hashmap *routes = NULL;

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
	int request_length = 40;
	void *request_data = malloc(request_length);

	memcpy(request_data,authority,16);
	memcpy(request_data + 16,peer,16);

	return new_BANG_request(request,request_data,request_length);
}

static BANG_request* create_request_with_message(enum BANG_request_types request, char *message) {
	return new_BANG_request(request,message,strlen(message));
}

static BANG_request* create_request_with_job(enum BANG_request_types request, uuid_t authority, uuid_t peer, BANG_job *job) {
	int request_length = job->length + 40;
	void *request_data = malloc(request_length);

	memcpy(request_data,authority,16);
	memcpy(request_data + 16,peer,16);
	memcpy(request_data + 32,&job->job_number,4);
	memcpy(request_data + 36,&job->length,4);
	memcpy(request_data + 40,&job->data,job->length);

	return new_BANG_request(request,request_data,request_length);
}

void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(routes != NULL);
	assert(job != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job(route->mr,job,authority,peer);

	} else {

		BANG_request *request = create_request_with_job(BANG_SEND_JOB_REQUEST,authority,peer,job);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_job_to_uuids(uuid_t authority, uuid_t *peers, BANG_job *job) {
	assert(routes != NULL);
	assert(job != NULL);

	int i;

	for (i = 0; !uuid_is_null(peers[i]); ++i) {
		BANG_route_job(authority,peers[i],job);
	}
}

void BANG_route_finished_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(routes != NULL);
	assert(job != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&authority);
	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_finished(route->mr,job,authority,peer);

	} else {

		BANG_request *request = create_request_with_job(BANG_SEND_FINISHED_JOB_REQUEST,authority,peer,job);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_request_job(uuid_t peer, uuid_t authority) {
	assert(routes != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&authority);
	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_request(route->mr,authority,peer);

	} else {

		BANG_request *request = create_request(BANG_SEND_REQUEST_JOB_REQUEST,authority,peer);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer) {
	assert(routes != NULL);
	assert(!uuid_is_null(authority));
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	if (route == NULL) return;

	if (route->remote == LOCAL) {
		BANG_module_callback_job_available(route->mr,authority,peer);

	} else {

		BANG_request *request = create_request(BANG_SEND_AVAILABLE_JOB_REQUEST,authority,peer);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

void BANG_route_send_message(uuid_t peer, char *message) {
	assert(routes != NULL);
	assert(message != NULL);
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	if (route == NULL) return;

	if (route->remote == LOCAL) {
		fprintf(stderr,"%s",message);

	} else {

		BANG_request *request = create_request_with_message(BANG_DEBUG_REQUEST,message);
		BANG_request_peer_id(route->pr->peer_id,request);
	}
}

int BANG_route_get_peer_id(uuid_t peer) {
	assert(routes != NULL);
	assert(!uuid_is_null(peer));

	peer_or_module *route = BANG_hashmap_get(routes,&peer);
	route = NULL;

	return -1;
}

int** BANG_not_route_get_peer_id(uuid_t *peers) {
	assert(routes != NULL);
	assert(peers != NULL);

	return NULL;
}

void BANG_route_new_peer(uuid_t peer, uuid_t new_peer) {
	assert(routes != NULL);
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(new_peer));
}

void BANG_route_remove_peer(uuid_t peer, uuid_t old_peer) {
	assert(routes != NULL);
	assert(!uuid_is_null(peer));
	assert(!uuid_is_null(old_peer));
}

void BANG_register_module_route(BANG_module *module) {
	assert(routes != NULL);
	assert(module != NULL);
}

void BANG_register_peer_route(uuid_t uuid, int peer_id, char *module_name, unsigned char* module_version) {
	peer_or_module *pom = new_pom_peer_route(peer_id,module_name,module_version);
	BANG_hashmap_set(routes,&uuid,pom);
}

void BANG_deregister_route(uuid_t route) {
	assert(!uuid_is_null(route));
}

void BANG_route_init() {
}

void BANG_route_close() {
}
