#ifndef HEAVENCOMMON_H
#define HEAVENCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define HV_DEFAULT_SOCKET "/tmp/heaven-0"

typedef u_int32_t UInt32;

struct hv_client_struct;
typedef struct hv_client_struct hv_client;

struct hv_compositor_struct;
typedef struct hv_compositor_struct hv_compositor;

struct hv_top_bar_struct;
typedef struct hv_top_bar_struct hv_top_bar;

struct hv_menu_struct;
typedef struct hv_menu_struct hv_menu;

struct hv_action_struct;
typedef struct hv_action_struct hv_action;

struct hv_separator_struct;
typedef struct hv_separator_struct hv_separator;

typedef void hv_object;
typedef void hv_item;

/* Array */

typedef struct hv_node_struct hv_node;

struct hv_node_struct
{
    hv_node *prev;
    hv_node *next;
    void *data;
};

typedef struct hv_array_struct hv_array;

struct hv_array_struct
{
    hv_node *begin;
    hv_node *end;
};


hv_array *hv_array_create();
void hv_array_destroy(hv_array *array);
void hv_array_pop_front(hv_array *array);
void hv_array_pop_back(hv_array *array);
int hv_array_empty(hv_array *array);
void hv_array_erase(hv_array *array, hv_node *node);
hv_node *hv_array_push_back(hv_array *array, void *data);
hv_node *hv_array_push_front(hv_array *array, void *data);
hv_node *hv_array_insert_before(hv_array *array, hv_node *before, void *data);


UInt32 hv_object_get_id(hv_object *object);

/* CONNECTION TYPES */

#define HV_CONNECTION_TYPE_COMPOSITOR 1
#define HV_CONNECTION_TYPE_CLIENT 2

/* CLIENT REQUESTS */

#define HV_OBJECT_CREATE_ID 1
#define HV_OBJECT_DESTROY_ID 2
#define HV_OBJECT_REMOVE_FROM_PARENT_ID 3
#define HV_CLIENT_SET_APP_NAME_ID 4
#define HV_TOP_BAR_SET_ACTIVE_ID 5
#define HV_MENU_SET_TITLE_ID 6
#define HV_MENU_ADD_TO_TOP_BAR_ID 7
#define HV_MENU_ADD_TO_ACTION_ID 8
#define HV_ACTION_SET_TEXT_ID 9

/* RESOURCE TYPES */

#define HV_OBJECT_TYPE_TOP_BAR 0
#define HV_OBJECT_TYPE_MENU 1
#define HV_OBJECT_TYPE_ACTION 2
#define HV_OBJECT_TYPE_SEPARATOR 3

/* RETURN CODES */

#define HV_SUCCESS 1
#define HV_ERROR 0


#endif // HEAVENCOMMON_H
