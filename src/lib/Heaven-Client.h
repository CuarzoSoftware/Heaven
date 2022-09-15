#ifndef HEAVENCLIENT_H
#define HEAVENCLIENT_H

/** @defgroup Heaven-Client Heaven Client API
 *
 * @brief API used by client applications to send their menus to the global menu server.
 *
 * @addtogroup Heaven-Client
 * @{
 */

#include "Heaven-Server-Client-Common.h"

typedef struct hn_client_events_interface_struct hn_client_events_interface;

struct hn_client_events_interface_struct
{
    void (*disconnected_from_server)(hn_client *);
    void (*object_destroy)(hn_object *);
    void (*server_action_invoke)(hn_action *);
    void (*server_toggle_set_checked)(hn_toggle *, hn_bool);
    void (*server_option_set_active)(hn_option *, hn_select *);
    void (*server_send_custom_event)(hn_client *, void *, u_int32_t size);
};

/*!
 * @brief Stablish a new connection with a global menu server.
 *
 * @param socket_name
 * Name of the server socket. If **NULL** "heaven-0" will be used.
 *
 * @param app_name
 * Name of the client application.
 *
 * @param user_data
 * Pointer to data which can later be obtained with the hn_client_get_user_data() menthod.
 *
 * @param events_interface
 * Pointer a hn_client_events_interface struct used to handle server events.
 *
 * @return Returns a pointer to a hn_client struct or NULL if failed.
 */
hn_client *hn_client_create(const char *socket_name, const char *app_name, void *user_data, hn_client_events_interface *events_interface);

/*!
 * \brief Dispatch events received from the global menu server.
 *
 * Blocks until a server event is received or timeout is reached and dispatches the events.
 *
 * \param client
 * Pointer to a valid hn_client struct returned by the hn_client_create() method.
 *
 * \param timeout
 * Number of milliseconds to block until an event is received. Passing the value -1 disables the timeout.
 *
 * \return
 * HN_SUCESS if events where received or timeout was reached.\n
 * HN_CONNECTION_LOST if the connection with the server was lost.
 */
int hn_client_dispatch_events(hn_client *client, int timeout);

/*!
 * @brief Closes the connection with the global menu server.
 *
 * This method closes the connection with the server and destroys the hn_client and all the hn_objects created during the session.\n
 * The **object_destroy** event is called for every hn_object before they are destroyed.\n
 *
 * @warning
 * This method is also called by the library when the connection with the server is lost
 * so it must not be called after a method returns HN_CONNECTION_LOST.
 *
 * @param client
 */
void hn_client_destroy(hn_client *client);
int hn_client_set_app_name(hn_client * client, const char *app_name);
int hn_client_get_fd(hn_client *client);
int hn_client_send_custom_request(hn_client *client, void *data, u_int32_t size);
const char *hn_client_get_error_string(hn_client *client);

/* Object */
int hn_object_remove_fom_parent(hn_object *object);
int hn_object_destroy(hn_object *object);

/* TOP BAR */
hn_top_bar *hn_top_bar_create(hn_client *client, void *user_data);
int hn_top_bar_set_active(hn_top_bar *top_bar);

/* MENU */
hn_menu *hn_menu_create(hn_client *client, void *user_data);
int hn_menu_set_icon(hn_menu *menu, hn_pixel *pixels, u_int32_t width, u_int32_t height);
int hn_menu_set_label(hn_menu *menu, const char *label);
int hn_menu_set_enabled(hn_menu *menu, hn_bool enabled);
int hn_menu_add_to_top_bar(hn_menu *menu, hn_top_bar *parent, hn_menu *before);
int hn_menu_add_to_menu(hn_menu *menu, hn_menu *parent, hn_object *before);

/* ACTION */
hn_action *hn_action_create(hn_client *client, void *user_data);
int hn_action_set_icon(hn_action *action, hn_pixel *pixels, u_int32_t width, u_int32_t height);
int hn_action_set_label(hn_action *action, const char *label);
int hn_action_set_enabled(hn_action *action, hn_bool enabled);
int hn_action_set_shortcuts(hn_action *action, const char *shortcuts);
int hn_action_add_to_menu(hn_action *action, hn_menu *parent, hn_object *before);

/* TOGGLE */
hn_toggle *hn_toggle_create(hn_client *client, void *user_data);
int hn_toggle_set_label(hn_toggle *toggle, const char *label);
int hn_toggle_set_enabled(hn_toggle *toggle, hn_bool enabled);
int hn_toggle_set_shortcuts(hn_toggle *toggle, const char *shortcuts);
int hn_toggle_set_checked(hn_toggle *toggle, hn_bool checked);
int hn_toggle_add_to_menu(hn_toggle *toggle, hn_menu *parent, hn_object *before);

/* SELECT */
hn_select *hn_select_create(hn_client *client, void *user_data);
int hn_select_add_to_menu(hn_select *select, hn_menu *parent, hn_object *before);

/* OPTION */
hn_option *hn_option_create(hn_client *client, void *user_data);
int hn_option_set_label(hn_option *option, const char *label);
int hn_option_set_enabled(hn_option *option, hn_bool enabled);
int hn_option_set_shortcuts(hn_option *option, const char *shortcuts);
int hn_option_set_active(hn_option *option);
int hn_option_add_to_select(hn_option *option, hn_select *parent, hn_object *before);

/* SEPARATOR */
hn_separator *hn_separator_create(hn_client *client, void *user_data);
int hn_separator_set_label(hn_separator *separator, const char *label);
int hn_separator_add_to_menu(hn_separator *separator, hn_menu *parent, hn_object *before);
int hn_separator_add_to_select(hn_separator *separator, hn_select *parent, hn_object *before);



/*! @}*/

#endif // HEAVENCLIENT_H
