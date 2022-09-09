#ifndef HEAVENSERVER_H
#define HEAVENSERVER_H

#include "../common/Heaven-Common.h"

#define HV_MAX_CLIENTS 128

typedef struct hv_server_struct hv_server;

typedef struct  hv_server_requests_interface_struct  hv_server_requests_interface;

struct hv_server_requests_interface_struct
{
    void (*client_connected)(hv_client *);
    void (*client_set_app_name)(hv_client *, const char *);
    void (*client_disconnected)(hv_client *);

    void (*object_create)(hv_object *object);
    void (*object_remove_from_parent)(hv_object *object);
    void (*object_destroy)(hv_object *object);

    void (*top_bar_set_active)(hv_top_bar *);

    void (*menu_set_title)(hv_menu *, const char *);
    void (*menu_add_to_top_bar)(hv_menu *, hv_top_bar *, hv_menu *);
    void (*menu_add_to_action)(hv_menu *, hv_action *);

    void (*action_set_icon)(hv_action *, const unsigned char *, UInt32, UInt32);
    void (*action_set_text)(hv_action *, const char *);
    void (*action_set_shortcuts)(hv_action *, const char *);

    void (*separator_set_text)(hv_separator *, const char *);

    void (*item_add_to_menu)(hv_item *, hv_menu *, hv_item *);

};

hv_server *hv_server_create(const char *socket, hv_server_requests_interface *events_interface);

int hv_server_get_fd(hv_server *server);

int hv_server_handle_requests(hv_server *server, int timeout);

void hv_client_destroy(hv_client *client);

#endif // HEAVENSERVER_H
