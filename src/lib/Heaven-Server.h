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

#define HN_MAX_CLIENTS 256

typedef struct hn_server_struct hn_server;

typedef struct  hn_server_requests_interface_struct  hn_server_requests_interface;

struct hn_server_requests_interface_struct
{
    void (*client_connected)(hn_client *);
    void (*client_set_app_name)(hn_client *, const char *);
    void (*client_send_custom_request)(hn_client *, void *, u_int32_t);
    void (*client_disconnected)(hn_client *);

    void (*client_object_create)(hn_client *, hn_object *);
    void (*client_object_set_parent)(hn_client *, hn_object *, hn_object *, hn_object *);
    void (*client_object_set_label)(hn_client *, hn_object *, const char *);
    void (*client_object_set_icon)(hn_client *, hn_object *, const hn_pixel *, u_int32_t, u_int32_t);
    void (*client_object_set_shortcuts)(hn_client *, hn_object *, const char *);
    void (*client_object_set_enabled)(hn_client *, hn_object *, hn_bool);
    void (*client_object_set_checked)(hn_client *, hn_object *, hn_bool);
    void (*client_object_set_active)(hn_client *, hn_object *);
    void (*client_object_destroy)(hn_client *, hn_object *);

    void (*compositor_connected)(hn_compositor *);
    void (*compositor_set_active_client)(hn_compositor*, hn_client*, hn_client_pid);
    void (*compositor_send_custom_request)(hn_compositor*, void*, u_int32_t);
    void (*compositor_disconnected)(hn_compositor *);

};

hn_server *hn_server_create(const char *socket, void *user_data, hn_server_requests_interface *events_interface);

void hn_server_set_user_data(hn_server *server, void *user_data);

void *hn_server_get_user_data(hn_server *server);

int hn_server_get_fd(hn_server *server);

int hn_server_dispatch_requests(hn_server *server, int timeout);

void hn_server_compositor_destroy(hn_compositor *compositor);
void hn_server_client_destroy(hn_client *client);

hn_client_id hn_client_get_id(hn_client *client);
hn_server *hn_compositor_get_server(hn_compositor *compositor);
hn_compositor *hn_server_get_compositor(hn_server *server);
hn_client *hn_client_get_by_pid(hn_server *server, hn_client_pid pid);

void hn_client_get_credentials(hn_client *client, pid_t *pid, uid_t *uid, gid_t *gid);
hn_server *hn_client_get_server(hn_client *client);

int hn_action_invoke(hn_action *action);
int hn_server_send_custom_event_to_client(hn_client *client, void *data, u_int32_t size);
int hn_server_send_custom_event_to_compositor(hn_compositor *compositor, void *data, u_int32_t size);

/*! @}*/

#endif // HEAVENSERVER_H

#ifdef __cplusplus
}
#endif
