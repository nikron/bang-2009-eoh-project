#ifndef __BANG_ROUTING_H
#define __BANG_ROUTING_H
#include<uuid/uuid.h>

void BANG_route_job(uuid_t uuid, BANG_job *job);

void BANG_route_job_to_uuids(uuid_t *uuids, BANG_job *job);

uuid_t BANG_register_route(BANG_callbacks callbacks, BANG_module_info *info);
#endif
