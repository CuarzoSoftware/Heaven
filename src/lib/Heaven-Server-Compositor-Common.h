#ifndef HEAVENSERVERCOMPOSITORCOMMON_H
#define HEAVENSERVERCOMPOSITORCOMMON_H

#include "Heaven-Common.h"

/*!
 * @defgroup Server-Compositor-Common Server-Compositor Common
 *
 * @brief Shared members between the server and compositor API.
 *
 * @addtogroup Server-Compositor-Common
 * @{
 */

/*!
 * @brief The process id of a client.
 */
typedef pid_t hv_client_pid;

typedef u_int8_t hv_compositor_auth_reply;

enum HV_COMPOSITOR_AUTH_REPLY
{
    HV_COMPOSITOR_ACCEPTED,
    HV_COMPOSITOR_REJECTED
};

/* COMPOSITOR REQUESTS */

enum HV_COMPOSITOR_REQUEST
{
    HV_SET_ACTIVE_CLIENT_ID,
    HV_COMPOSITOR_SEND_CUSTOM_REQUEST_ID
};

/* SERVER EVENTS */

enum HV_SERVER_TO_COMPOSITOR_EVENTS
{
    HV_SERVER_TO_COMPOSITOR_SEND_CUSTOM_EVENT_ID,
};


/*!
 * @struct hv_compositor
 * @brief Representation of a compositor connected to the global menu server.
 */
struct hv_compositor_struct;
typedef struct hv_compositor_struct hv_compositor;

void hv_compositor_ser_user_data(hv_compositor *compositor, void *user_data);
void *hv_compositor_get_user_data(hv_compositor *compositor);


/*! @}*/

#endif // HEAVENSERVERCOMPOSITORCOMMON_H
