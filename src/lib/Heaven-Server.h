#ifndef HEAVENSERVER_H
#define HEAVENSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup Heaven-Server Heaven Server API
 *
 * @brief API used by global menu processes to receive and display clients menus and notify action events.
 *
 * @addtogroup Heaven-Server
 * @{
 */

#include "Heaven-Server-Client-Common.h"
#include "Heaven-Server-Compositor-Common.h"

#define HV_MAX_CLIENTS 256

typedef struct hv_server_struct hv_server;

typedef struct  hv_server_requests_interface_struct  hv_server_requests_interface;

struct hv_server_requests_interface_struct
{
    void (*client_connected)(hv_client *);
    void (*client_set_app_name)(hv_client *, const char *);
    void (*client_send_custom_request)(hv_client *, void *, u_int32_t);
    void (*client_disconnected)(hv_client *);

    void (*client_object_create)(hv_client *, hv_object *object);
    void (*client_object_remove_from_parent)(hv_client *, hv_object *object);
    void (*client_object_destroy)(hv_client *, hv_object *object);

    void (*client_top_bar_set_active)(hv_client *, hv_top_bar *);

    void (*client_menu_set_title)(hv_client *, hv_menu *, const char *);
    void (*client_menu_add_to_top_bar)(hv_client *, hv_menu *, hv_top_bar *, hv_menu *);
    void (*client_menu_add_to_action)(hv_client *, hv_menu *, hv_action *);

    void (*client_action_set_icon)(hv_client *, hv_action *, const hv_pixel *, u_int32_t, u_int32_t);
    void (*client_action_set_text)(hv_client *, hv_action *, const char *);
    void (*client_action_set_shortcuts)(hv_client *, hv_action *, const char *);
    void (*client_action_set_state)(hv_client *, hv_action *, hv_action_state);

    void (*client_separator_set_text)(hv_client *, hv_separator *, const char *);

    void (*client_item_add_to_menu)(hv_client *, hv_item *, hv_menu *, hv_item *);

    void (*compositor_connected)(hv_compositor *);
    void (*compositor_set_active_client)(hv_compositor*, hv_client*, hv_client_pid);
    void (*compositor_send_custom_request)(hv_compositor*, void*, u_int32_t);
    void (*compositor_disconnected)(hv_compositor *);

};

hv_server *hv_server_create(const char *socket, void *user_data, hv_server_requests_interface *events_interface);

void hv_server_set_user_data(hv_server *server, void *user_data);

void *hv_server_get_user_data(hv_server *server);

int hv_server_get_fd(hv_server *server);

int hv_server_dispatch_requests(hv_server *server, int timeout);

void hv_server_compositor_destroy(hv_compositor *compositor);
void hv_server_client_destroy(hv_client *client);

hv_client_id hv_client_get_id(hv_client *client);
hv_server *hv_compositor_get_server(hv_compositor *compositor);
hv_compositor *hv_server_get_compositor(hv_server *server);
hv_client *hv_client_get_by_pid(hv_server *server, hv_client_pid pid);

void hv_client_get_credentials(hv_client *client, pid_t *pid, uid_t *uid, gid_t *gid);
hv_server *hv_client_get_server(hv_client *client);

int hv_action_invoke(hv_action *action);
int hv_server_send_custom_event_to_client(hv_client *client, void *data, u_int32_t size);
int hv_server_send_custom_event_to_compositor(hv_compositor *compositor, void *data, u_int32_t size);

/*! @}*/

#endif // HEAVENSERVER_H

#ifdef __cplusplus
}
#endif
