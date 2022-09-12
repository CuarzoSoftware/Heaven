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

typedef struct hv_client_events_interface_struct hv_client_events_interface;

struct hv_client_events_interface_struct
{
    void (*disconnected_from_server)(hv_client *);
    void (*object_destroy)(hv_object *);
    void (*server_action_invoke)(hv_action *);
    void (*server_send_custom_event)(hv_client *, void *, u_int32_t size);
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
 * Pointer to data which can later be obtained with the hv_client_get_user_data() menthod.
 *
 * @param events_interface
 * Pointer a hv_client_events_interface struct used to handle server events.
 *
 * @return Returns a pointer to a hv_client struct or NULL if failed.
 */
hv_client *hv_client_create(const char *socket_name, const char *app_name, void *user_data, hv_client_events_interface *events_interface);

/*!
 * \brief Dispatch events received from the global menu server.
 *
 * Blocks until a server event is received or timeout is reached and dispatches the events.
 *
 * \param client
 * Pointer to a valid hv_client struct returned by the hv_client_create() method.
 *
 * \param timeout
 * Number of milliseconds to block until an event is received. Passing the value -1 disables the timeout.
 *
 * \return
 * HV_SUCESS if events where received or timeout was reached.\n
 * HV_CONNECTION_LOST if the connection with the server was lost.
 */
int hv_client_dispatch_events(hv_client *client, int timeout);

/*!
 * @brief Closes the connection with the global menu server.
 *
 * This method closes the connection with the server and destroys the hv_client and all the hv_objects created during the session.\n
 * The **object_destroy** event is called for every hv_object before they are destroyed.\n
 *
 * @warning
 * This method is also called by the library when the connection with the server is lost
 * so it must not be called after a method returns HV_CONNECTION_LOST.
 *
 * @param client
 */
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

/*! @}*/

#endif // HEAVENCLIENT_H
