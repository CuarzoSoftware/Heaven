#include "../../lib/Heaven-Compositor.h"
#include <sys/poll.h>
#include <string.h>

static void disconnected_from_server(hv_compositor *compositor)
{
    HV_UNUSED(compositor);
    printf("- DISCONNECTED FROM SERVER\n");
}

static void server_send_custom_event(hv_compositor *compositor, void *data, u_int32_t size)
{
    HV_UNUSED(compositor);
    char *msg = data;
    msg[size] = '\0';
    printf("- SERVER SENT A CUSTOM MESSAGE = \"%s\"\n", msg);
}

hv_compositor_events_interface events_interface =
{
    &disconnected_from_server,
    &server_send_custom_event
};

int main()
{

    hv_compositor *compositor = hv_compositor_create(NULL, NULL, &events_interface);

    if(!compositor)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("\nCompositor started.\n\n");

    char *msg = "Hello dear server!";
    hv_compositor_send_custom_request(compositor, msg, strlen(msg) + 1);

    sleep(1);

    /* Listen to the global menu events*/

    while(1)
    {
        // Wait 500 ms for a server response (pass -1 to block until event or disconnection)
        if(hv_compositor_dispatch_events(compositor, 500) == HV_CONNECTION_LOST)
            break;

        hv_client_pid pid;
        printf("\nEnter a client proccess ID or 0: ");
        scanf("%d", &pid);
        printf("\n");
        hv_compositor_set_active_client(compositor, pid);
    }

    /*
     * POLLING EXAMPLE
     *
    struct pollfd fd;
    fd.fd = hv_compositor_get_fd(compositor);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        // Pass 0 to return immediately
        if(hv_compositor_dispatch_events(compositor, 0) == HV_CONNECTION_LOST)
            break;
    }
    */

    return 0;
}
