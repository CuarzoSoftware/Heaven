#include "../../lib/server/Heaven-Server.h"
#include <poll.h>

void client_connected_request(struct hv_client *client)
{
    printf("Client connected.\n");
}

void client_disconnected_request(struct hv_client *client)
{
    printf("Client disconnected.\n");
}

void client_set_app_name_request(struct hv_client *client, const char *app_name)
{
    printf("Client app name: %s\n", app_name);
}

struct hv_server_requests_interface events_interface =
{
    .client_connected_request = &client_connected_request,
    .client_disconnected_request = &client_disconnected_request,
    .client_set_app_name_request = &client_set_app_name_request
};

int main()
{

    struct hv_server *server = hv_create_server(NULL, &events_interface);

    if(!server)
    {
        printf("Error: Could not create server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Server started.\n");

    struct pollfd fd;
    fd.fd = hv_server_get_fd(server);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        hv_server_handle_requests(server, 0);
    }

    return 0;
}
