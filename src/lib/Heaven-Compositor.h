/** @defgroup Heaven-Compositor Heaven Compositor API
 *
 * @brief API used by Wayland or X11 compositors to send the pid of the current active client to the global menu server\n\n
 *
 * @addtogroup Heaven-Compositor
 * @{
 */

#include "Heaven-Server-Compositor-Common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hn_compositor_events_interface_struct hn_compositor_events_interface;

struct hn_compositor_events_interface_struct
{
    void (*disconnected_from_server)(hn_compositor *);
    void (*server_send_custom_event)(hn_compositor *, void *, u_int32_t size);
};

hn_compositor *hn_compositor_create(const char *socket_name, void *user_data, hn_compositor_events_interface *events_interface);
int hn_compositor_dispatch_events(hn_compositor *compositor, int timeout);
int hn_compositor_get_fd(hn_compositor *compositor);
void hn_compositor_destroy(hn_compositor *compositor);


/* REQUESTS */
int hn_compositor_set_active_client(hn_compositor *compositor, hn_client_pid client_pid);
int hn_compositor_send_custom_request(hn_compositor *compositor, void *data, u_int32_t size);

#ifdef __cplusplus
}
#endif

/*! @}*/
