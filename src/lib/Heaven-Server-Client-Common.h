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

#ifdef __cplusplus
extern "C" {
#endif

/*! A value containing a client id. */
typedef u_int16_t hn_client_id;

/*! A value containing an object id. */
typedef u_int16_t hn_object_id;

/*! A value containing an object type. */
typedef u_int8_t hn_object_type;

/*! A value containing an 8-bit pixel. */
typedef u_int8_t hn_pixel;


/*!
 *  @brief States of an action.
 *
 *  When an hn_action is created its default state is enabled.
 */
enum HN_ACTION_STATE
{
    /*! The action is disabled and therefore can not be invoked. */
    HN_ACTION_STATE_DISABLED = 0,

    /*! The action is enabled therefore can be invoked. */
    HN_ACTION_STATE_ENABLED = 1
};

/*!
 *  @brief Object types.
 */
enum HN_OBJECT_TYPE
{
    /*! Top bar */
    HN_OBJECT_TYPE_TOP_BAR,

    /*! Menu */
    HN_OBJECT_TYPE_MENU,

    /*! Action */
    HN_OBJECT_TYPE_ACTION,

    /*! Toggle */
    HN_OBJECT_TYPE_TOGGLE,

    /*! Select */
    HN_OBJECT_TYPE_SELECT,

    /*! A select option */
    HN_OBJECT_TYPE_OPTION,

    /*! Separator */
    HN_OBJECT_TYPE_SEPARATOR,
};

/* CLIENT REQUESTS */

enum HN_CLIENT_REQUEST
{
    HN_CLIENT_REQUEST_OBJECT_CREATE_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_PARENT_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_ICON_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_LABEL_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_SHORTCUTS_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_ENABLED_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_CHECKED_ID,
    HN_CLIENT_REQUEST_OBJECT_SET_ACTIVE_ID,
    HN_CLIENT_REQUEST_OBJECT_DESTROY_ID,
    HN_CLIENT_REQUEST_SET_APP_NAME_ID,
    HN_CLIENT_REQUEST_SEND_CUSTOM_REQUEST_ID,
};

/* SERVER EVENTS */

enum HN_SERVER_TO_CLIENT_EVENTS
{
    HN_ACTION_INVOKE_ID,
    HN_SERVER_TO_CLIENT_SEND_CUSTOM_EVENT_ID,
};

/*!
 * @struct hn_client
 * @brief Representation of a client connected to the global menu server.
 */

struct hn_client_struct;
typedef struct hn_client_struct hn_client;

/*!
 * @struct hn_top_bar
 * @brief A container of an ordered group of menus (e.g. File, Edit, View, etc) tipically displayed at the top of the screen.
 * Can be interpreted as a hn_object and have multiple hn_menu childs but no parent.
 */
struct hn_top_bar_struct;
typedef struct hn_top_bar_struct hn_top_bar;

struct hn_menu_struct;
typedef struct hn_menu_struct hn_menu;

struct hn_action_struct;
typedef struct hn_action_struct hn_action;

struct hn_separator_struct;
typedef struct hn_separator_struct hn_separator;

struct hn_toggle_struct;
typedef struct hn_toggle_struct hn_toggle;

struct hn_select_struct;
typedef struct hn_select_struct hn_select;

struct hn_option_struct;
typedef struct hn_toggle_struct hn_option;

typedef void hn_object;

void *hn_client_get_user_data(hn_client *client);
void hn_client_set_user_data(hn_client *client, void *user_data);
hn_object *hn_object_get_by_id(hn_client *client, hn_object_id id);
hn_object_id hn_object_get_id(hn_object *object);
hn_object_type hn_object_get_type(hn_object *object);
hn_client *hn_object_get_client(hn_object *object);
hn_object *hn_object_get_parent(hn_object *object);
hn_object *hn_object_get_user_data(hn_object *object);
void hn_object_set_user_data(hn_object *object, void *user_data);

/*! @}*/

#endif // HEAVENSERVERCLIENTCOMMON_H
