#ifndef HEAVENSERVERCOMPOSITORCOMMON_H
#define HEAVENSERVERCOMPOSITORCOMMON_H

#include "Heaven-Common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
typedef pid_t hn_client_pid;

typedef u_int8_t hn_compositor_auth_reply;

enum HN_COMPOSITOR_AUTH_REPLY
{
    HN_COMPOSITOR_ACCEPTED,
    HN_COMPOSITOR_REJECTED
};

/* COMPOSITOR REQUESTS */

enum HN_COMPOSITOR_REQUEST
{
    HN_SET_ACTIVE_CLIENT_ID,
    HN_COMPOSITOR_SEND_CUSTOM_REQUEST_ID
};

/* SERVER EVENTS */

enum HN_SERVER_TO_COMPOSITOR_EVENTS
{
    HN_SERVER_TO_COMPOSITOR_SEND_CUSTOM_EVENT_ID,
};


/*!
 * @struct hn_compositor
 * @brief Representation of a compositor connected to the global menu server.
 */
struct hn_compositor_struct;
typedef struct hn_compositor_struct hn_compositor;

void hn_compositor_ser_user_data(hn_compositor *compositor, void *user_data);
void *hn_compositor_get_user_data(hn_compositor *compositor);


/*! @}*/

#ifdef __cplusplus
}
#endif

#endif // HEAVENSERVERCOMPOSITORCOMMON_H



