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

} peer_route;

typedef struct {
	enum remote_route remote;
	peer_route *remote;
} peer_or_module;

static BANG_hashmap *routes = NULL;

void BANG_route_job(uuid_t authority, uuid_t peer, BANG_job *job) {
	assert(routes != NULL);
	assert(job != NULL);
	
}

void BANG_route_job_to_uuids(uuid_t authority, uuid_t *peers, BANG_job *job) {
}

void BANG_route_finished_job(uuid_t auth, uuid_t peer, BANG_job *job) {
}

void BANG_route_request_job(uuid_t peer, uuid_t authority) {
}

void BANG_route_assertion_of_authority(uuid_t authority, uuid_t peer) {
}

void BANG_route_send_message(uuid_t uuid, char *message) {
}

int BANG_route_get_peer_id(uuid_t uuid) {
}

int** BANG_not_route_get_peer_id(uuid_t *uuids) {
	return NULL:
}

void BANG_route_new_peer(uuid_t peer, uuid_t new_peer) {
}

void BANG_route_remove_peer(uuid_t peer, uuid_t old_peer) {
}

void BANG_register_module_route(BANG_module *module) {
}

void BANG_register_peer_route(uuid_t uuid, int peer, char *module_name, unsigned char* module_version) {
}

void BANG_deregister_route(uuid_t uid) {
}

void BANG_route_init() {
}

void BANG_route_close() {
}
#endif
