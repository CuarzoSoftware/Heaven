#ifndef HEAVENCLIENT_H
#define HEAVENCLIENT_H

#include "Heaven-Common.h"

typedef struct hv_client_events_interface_struct hv_client_events_interface;

struct hv_client_events_interface_struct
{
    void (*disconnected_from_server)(hv_client *);
    void (*object_destroy)(hv_object *);
    void (*server_action_invoke)(hv_action *);
    void (*server_send_custom_event)(hv_client *, void *, u_int32_t size);
};

/* CLIENT */
hv_client *hv_client_create(const char *socket_name, const char *app_name, void *user_data, hv_client_events_interface *events_interface);
int hv_client_dispatch_events(hv_client *client, int timeout);
void hv_client_destroy(hv_client *client);
int hv_client_set_app_name(hv_client * client, const char *app_name);
int hv_client_get_fd(hv_client *client);
int hv_client_send_custom_request(hv_client *client, void *data, u_int32_t size);

/* Object */
hv_object *hv_object_create(hv_client * client, hv_object_type type,  void *user_data);
int hv_object_destroy(hv_object *object);
int hv_object_remove_from_parent(hv_object *object);

/* MENU BAR */
int hv_top_bar_set_active(hv_top_bar *top_bar);

/* MENU */
int hv_menu_set_title(hv_menu * menu, const char *title);
int hv_menu_add_to_top_bar(hv_menu *menu, hv_top_bar *top_bar, hv_menu *before);
int hv_menu_add_to_action(hv_menu *menu, hv_action *action);

/* ACTION */
int hv_action_set_icon(hv_action *action, hv_pixel *pixels, u_int32_t width, u_int32_t height);
int hv_action_set_text(hv_action *action, const char *text);
int hv_action_set_shortcuts(hv_action *action, const char *shortcuts);
int hv_action_set_state(hv_action *action, hv_action_state state);

/* SEPARATOR */
int hv_separator_set_text(hv_separator *separator, const char *text);

/* ITEM (ACTION or SEPARATOR) */
int hv_item_add_to_menu(hv_item *item, hv_menu *menu, hv_item *before);

#endif // HEAVENCLIENT_H
