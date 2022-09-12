#ifndef HEAVENSERVERCLIENTCOMMON_H
#define HEAVENSERVERCLIENTCOMMON_H

/*!
 * @defgroup Server-Client-Common Server-Client Common
 *
 * @brief Shared members between the server and client API.
 *
 * @addtogroup Server-Client-Common
 * @{
 */

#include "Heaven-Common.h"

/*! A value containing a client id. */
typedef u_int16_t hv_client_id;

/*! A value containing an object id. */
typedef u_int16_t hv_object_id;

/*! A value containing an object type. */
typedef u_int8_t hv_object_type;

/*! A value containing an 8-bit pixel. */
typedef u_int8_t hv_pixel;

/*! A value containing the state of an action. */
typedef u_int8_t hv_action_state;

/*!
 *  @brief States of an action.
 *
 *  When an hv_action is created its default state is enabled.
 */
enum HV_ACTION_STATE
{
    /*! The action is disabled and therefore can not be invoked. */
    HV_ACTION_STATE_DISABLED = 0,

    /*! The action is enabled therefore can be invoked. */
    HV_ACTION_STATE_ENABLED = 1
};

/*!
 *  @brief Object types.
 */
enum HV_OBJECT_TYPE
{
    /*! Top bar */
    HV_OBJECT_TYPE_TOP_BAR = 0,

    /*! Menu */
    HV_OBJECT_TYPE_MENU = 1,

    /*! Action */
    HV_OBJECT_TYPE_ACTION = 2,

    /*! Separator */
    HV_OBJECT_TYPE_SEPARATOR = 3
};

/* CLIENT REQUESTS */

enum HV_CLIENT_REQUEST
{
    HV_OBJECT_CREATE_ID,
    HV_OBJECT_DESTROY_ID,
    HV_OBJECT_REMOVE_FROM_PARENT_ID,
    HV_CLIENT_SET_APP_NAME_ID,
    HV_CLIENT_SEND_CUSTOM_REQUEST_ID,
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

/* SERVER EVENTS */

enum HV_SERVER_TO_CLIENT_EVENTS
{
    HV_ACTION_INVOKE_ID,
    HV_SERVER_TO_CLIENT_SEND_CUSTOM_EVENT_ID,
};

/*!
 * @struct hv_client
 * @brief Representation of a client connected to the global menu server.
 */
typedef struct hv_client_struct hv_client;
struct hv_client_struct;

/*!
 * @struct hv_top_bar
 * @brief A container of an ordered group of menus (e.g. File, Edit, View, etc) tipically displayed at the top of the screen.
 * Can be interpreted as a hv_object and have multiple hv_menu childs but no parent.
 */
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

void *hv_client_get_user_data(hv_client *client);
void hv_client_set_user_data(hv_client *client, void *user_data);
hv_object *hv_object_get_by_id(hv_client *client, hv_object_id id);
hv_object_id hv_object_get_id(hv_object *object);
hv_object_type hv_object_get_type(hv_object *object);
hv_client *hv_object_get_client(hv_object *object);
hv_object *hv_object_get_parent(hv_object *object);
hv_object *hv_object_get_user_data(hv_object *object);
void hv_object_set_user_data(hv_object *object, void *user_data);

/*! @}*/

#endif // HEAVENSERVERCLIENTCOMMON_H
