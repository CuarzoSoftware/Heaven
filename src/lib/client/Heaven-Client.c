#include "Heaven-Client.h"
#include <poll.h>

struct hv_client
{
    struct pollfd fd;
    struct sockaddr_un name;
    struct hv_client_events_interface *events_interface;
};

struct hv_client *hv_create_client(const char *socket_name, const char *app_name, struct hv_client_events_interface *events_interface)
{
    const char *sock_name;

    if(socket_name)
        sock_name = socket_name;
    else
        sock_name = HV_DEFAULT_SOCKET;

    struct hv_client *client = malloc(sizeof(struct hv_client));
    client->events_interface = events_interface;
    client->fd.events = POLLIN;
    client->fd.revents = 0;

    if( (client->fd.fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(client);
        return NULL;
    }

    memset(&client->name.sun_path, 0, 108);
    client->name.sun_family = AF_UNIX;
    strncpy(client->name.sun_path, sock_name, 107);


    if(connect(client->fd.fd, (const struct sockaddr*)&client->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: No server found in %s.\n", sock_name);
        free(client);
        return NULL;
    }

    if(hv_client_set_app_name(client, app_name))
    {
        printf("Error: App name is NULL.\n");
        close(client->fd.fd);
        free(client);
        return NULL;
    }

    return client;
}

int hv_client_set_app_name(struct hv_client *client, const char *app_name)
{
    if(app_name == NULL)
        return 1;

    u_int32_t msg_id = HV_CLIENT_SET_APP_NAME_ID;
    u_int32_t app_name_len = strlen(app_name);

    write(client->fd.fd, &msg_id, sizeof(msg_id));
    write(client->fd.fd, &app_name_len, sizeof(app_name_len));
    write(client->fd.fd, app_name, app_name_len);

    return 0;

}
