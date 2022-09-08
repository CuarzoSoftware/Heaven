#ifndef HEAVENCOMMON_H
#define HEAVENCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef u_int32_t UInt32;

#define HV_DEFAULT_SOCKET "/tmp/heaven-0"

struct hv_client;
struct hv_compositor;

struct hv_object;
struct hv_menu_bar;
struct hv_menu;

/* Array */

struct hv_node
{
    struct hv_node *prev;
    struct hv_node *next;
    void *data;
};

struct hv_array
{
    struct hv_node *begin;
    struct hv_node *end;
};

struct hv_array *hv_array_create();
void hv_array_destroy(struct hv_array *array);
void hv_array_pop_front(struct hv_array *array);
void hv_array_pop_back(struct hv_array *array);
int hv_array_empty(struct hv_array *array);
void hv_array_erase(struct hv_array *array, struct hv_node *node);
struct hv_node *hv_array_push_back(struct hv_array *array, void *data);


UInt32 hv_object_get_id(struct hv_object *object);

/* CONNECTION TYPES */

#define HV_CONNECTION_TYPE_COMPOSITOR 1
#define HV_CONNECTION_TYPE_CLIENT 2

/* COMMON REQUESTS */

#define HV_COMMON_IDENTIFY_ID 1

/* CLIENT REQUESTS */

#define HV_CLIENT_SET_APP_NAME_ID 2
#define HV_MENU_BAR_CREATE_ID 3
#define HV_MENU_BAR_DESTROY_ID 4
#define HV_MENU_CREATE_ID 5
#define HV_MENU_SET_TITLE_ID 6
#define HV_MENU_DESTROY_ID 7

/* RESOURCE TYPES */

#define HV_OBJECT_TYPE_MENU_BAR 0
#define HV_OBJECT_TYPE_MENU 1
#define HV_OBJECT_TYPE_ITEM 2
#define HV_OBJECT_TYPE_SEPARATOR 3

#endif // HEAVENCOMMON_H
