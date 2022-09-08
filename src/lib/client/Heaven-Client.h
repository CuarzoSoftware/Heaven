#ifndef HEAVENCLIENT_H
#define HEAVENCLIENT_H

#include "../common/Heaven-Common.h"

struct hv_client_events_interface
{
    void (*test)(const char*);
};

/* CLIENT */
struct hv_client *hv_client_create(const char *socket_name, const char *app_name, struct hv_client_events_interface *events_interface);
void hv_client_destroy(struct hv_client *client);
int hv_client_set_app_name(struct hv_client * client, const char *app_name);

/* MENU BAR */
struct hv_menu_bar *hv_menu_bar_create(struct hv_client * client, void *user_data);
void hv_menu_bar_destroy(struct hv_menu_bar *menu_bar);

/* MENU */
struct hv_menu *hv_menu_create(struct hv_client * client, void *user_data);
int hv_menu_set_title(struct hv_menu * menu, const char *title);
void hv_menu_destroy(struct hv_menu *menu);

#endif // HEAVENCLIENT_H
