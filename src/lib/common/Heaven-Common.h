#ifndef HEAVENCOMMON_H
#define HEAVENCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define HV_DEFAULT_SOCKET "/tmp/heaven-0"

#define HV_UNUSED(variable)(void)variable;

typedef u_int16_t hv_client_id;
typedef u_int16_t hv_object_id;
typedef u_int8_t hv_object_type;
typedef u_int8_t hv_connection_type;
typedef u_int8_t hv_message_id;
typedef u_int8_t hv_string_length;
typedef u_int8_t hv_pixel;
typedef u_int8_t hv_action_state;

/* CONNECTION TYPES */

enum HV_CONNECTION_TYPE
{
    HV_CONNECTION_TYPE_COMPOSITOR = 1,
    HV_CONNECTION_TYPE_CLIENT = 2
};

/* OBJECT TYPES */

enum HV_OBJECT_TYPE
{
    HV_OBJECT_TYPE_TOP_BAR = 0,
    HV_OBJECT_TYPE_MENU = 1,
    HV_OBJECT_TYPE_ACTION = 2,
    HV_OBJECT_TYPE_SEPARATOR = 3
};

/* ACTION STATES */

enum HV_ACTION_STATE
{
    HV_ACTION_STATE_DISABLED = 0,
    HV_ACTION_STATE_ENABLED = 1
};

/* CLIENT REQUESTS */

enum HV_CLIENT_REQUEST
{
    HV_OBJECT_CREATE_ID,
    HV_OBJECT_DESTROY_ID,
    HV_OBJECT_REMOVE_FROM_PARENT_ID,
    HV_CLIENT_SET_APP_NAME_ID,
    HV_TOP_BAR_SET_ACTIVE_ID,
    HV_MENU_SET_TITLE_ID,
    HV_MENU_ADD_TO_TOP_BAR_ID,
    HV_MENU_ADD_TO_ACTION_ID,
    HV_ACTION_SET_ICON_ID,
    HV_ACTION_SET_TEXT_ID,
    HV_ACTION_SET_SHORTCUTS_ID,
    HV_ACTION_SET_STATE_ID,
    HV_SEPARATOR_SET_TEXT_ID,
    HV_ITEM_ADD_TO_MENU_ID,
};

/* RETURN CODES */

#define HV_SUCCESS 1
#define HV_ERROR 0

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

hv_object_id hv_object_get_id(hv_object *object);
hv_object_type hv_object_get_type(hv_object *object);
hv_client *hv_object_get_client(hv_object *object);
hv_object *hv_object_get_parent(hv_object *object);


#endif // HEAVENCOMMON_H
