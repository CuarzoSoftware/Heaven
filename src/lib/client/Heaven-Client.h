#ifndef HEAVENCLIENT_H
#define HEAVENCLIENT_H

#include "../common/Heaven-Common.h"

struct hv_client_events_interface
{
    void (*test)(const char*);
};

struct hv_client *hv_create_client(const char *socket_name, const char *app_name, struct hv_client_events_interface *events_interface);
int hv_client_set_app_name(struct hv_client * client, const char *app_name);

#endif // HEAVENCLIENT_H
