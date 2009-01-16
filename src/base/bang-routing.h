#ifndef __BANG_ROUTING_H
#define __BANG_ROUTING_H
#include<uuid/uuid.h>

void BANG_route_to_uuid(uuid_t uuid, BANG_request request);

void BANG_route_to_uuids(uuid_t *uuids, BANG_request request);

uuid_t BANG_register_route(BANG_callbacks callbacks, BANG_module_info *info);
#endif
