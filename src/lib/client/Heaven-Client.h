#ifndef HEAVENCLIENT_H
#define HEAVENCLIENT_H

#include "../common/Heaven-Common.h"

typedef struct hv_client_events_interface_struct hv_client_events_interface;

struct hv_client_events_interface_struct
{
    void (*test)(const char*);
};


/* CLIENT */
hv_client *hv_client_create(const char *socket_name, const char *app_name, hv_client_events_interface *events_interface);
void hv_client_destroy(hv_client *client);
int hv_client_set_app_name(hv_client * client, const char *app_name);

/* Object */
int hv_object_destroy(hv_object *object);
int hv_object_remove_from_parent(hv_object *object);

/* MENU BAR */
hv_top_bar *hv_top_bar_create(hv_client * client, void *user_data);
int hv_top_bar_set_active(hv_top_bar *top_bar);

/* MENU */
hv_menu *hv_menu_create(hv_client * client, void *user_data);
int hv_menu_set_title(hv_menu * menu, const char *title);
int hv_menu_add_to_top_bar(hv_menu *menu, hv_top_bar *top_bar, hv_menu *before);
int hv_menu_add_to_action(hv_menu *menu, hv_action *action);

/* ACTION */
hv_action *hv_action_create(hv_client * client, void *user_data);
int hv_action_set_text(hv_action *action, const char *text);

#endif // HEAVENCLIENT_H
