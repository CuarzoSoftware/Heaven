#ifndef HEAVENSERVER_H
#define HEAVENSERVER_H

#include "../common/Heaven-Common.h"

#define HV_MAX_CLIENTS 128

struct hv_server;

struct hv_server_requests_interface
{
    void (*client_connected)(struct hv_client *);
    void (*client_disconnected)(struct hv_client *);
    void (*client_set_app_name_request)(struct hv_client *, const char *);

    void (*menu_bar_create_request)(struct hv_menu_bar *);
    void (*menu_bar_destroy_request)(struct hv_menu_bar *);

    void (*menu_create_request)(struct hv_menu *);
    void (*menu_set_title_request)(struct hv_menu *, const char *);
    void (*menu_destroy_request)(struct hv_menu *);
};

struct hv_server *hv_server_create(const char *socket, struct hv_server_requests_interface *events_interface);

int hv_server_get_fd(struct hv_server *server);

int hv_server_handle_requests(struct hv_server *server, int timeout);

void hv_client_destroy(struct hv_client *client);

#endif // HEAVENSERVER_H
