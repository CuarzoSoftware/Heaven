/** @defgroup Heaven-Compositor Heaven Compositor API
 *
 * @brief API used by Wayland or X11 compositors to send the pid of the current active client to the global menu server\n\n
 *
 * @addtogroup Heaven-Compositor
 * @{
 */

#include "Heaven-Server-Compositor-Common.h"

typedef struct hv_compositor_events_interface_struct hv_compositor_events_interface;

struct hv_compositor_events_interface_struct
{
    void (*disconnected_from_server)(hv_compositor *);
    void (*server_send_custom_event)(hv_compositor *, void *, u_int32_t size);
};

hv_compositor *hv_compositor_create(const char *socket_name, void *user_data, hv_compositor_events_interface *events_interface);
int hv_compositor_dispatch_events(hv_compositor *compositor, int timeout);
int hv_compositor_get_fd(hv_compositor *compositor);
void hv_compositor_destroy(hv_compositor *compositor);


/* REQUESTS */
int hv_compositor_set_active_client(hv_compositor *compositor, hv_client_pid client_pid);
int hv_compositor_send_custom_request(hv_compositor *compositor, void *data, u_int32_t size);


/*! @}*/
