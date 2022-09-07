#ifndef HEAVENSERVER_H
#define HEAVENSERVER_H

#include "../common/Heaven-Common.h"

#define HV_MAX_CLIENTS 128

struct hv_server;

struct hv_server_requests_interface
{
    void (*client_connected_request)(struct hv_client *);
    void (*client_disconnected_request)(struct hv_client *);
    void (*client_set_app_name_request)(struct hv_client *, const char *);
    void (*client_create_menu_bar_request)(struct hv_client *, struct hv_menu_bar *);
};

struct hv_server *hv_create_server(const char *socket, struct hv_server_requests_interface *events_interface);

int hv_server_get_fd(struct hv_server *server);

int hv_server_handle_requests(struct hv_server *server, int timeout);

void hv_client_destroy(struct hv_client *client);

#endif // HEAVENSERVER_H
