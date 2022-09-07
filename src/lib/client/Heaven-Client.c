#include "Heaven-Client.h"
#include <poll.h>

struct hv_client
{
    struct pollfd fd;
    struct sockaddr_un name;
    struct hv_client_events_interface *events_interface;
    struct hv_array *menu_bars;
    struct hv_array *freed_menu_bar_ids;
    u_int32_t greatest_menu_bar_id;
};

struct hv_menu_bar
{
    int id;
    struct hv_node *link;
    void *user_data;
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

    client->menu_bars = hv_array_create();
    client->freed_menu_bar_ids = hv_array_create();

    return client;
}

int hv_client_set_app_name(struct hv_client *client, const char *app_name)
{
    if(app_name == NULL)
        return 1;

    u_int32_t msg_id = HV_CLIENT_SET_APP_NAME_ID;
    u_int32_t app_name_len = strlen(app_name);

    /* MSG ID: UInt32
     * APP NAME LENGHT: UInt32
     * APP NAME: APP NAME LENGHT */

    int total_bytes = sizeof(u_int32_t)*2 + app_name_len;
    char buff[total_bytes];

    int offset = 0;
    memcpy(&buff[offset], &msg_id, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);
    memcpy(&buff[offset], &app_name_len, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);
    memcpy(&buff[offset], app_name, app_name_len);

    write(client->fd.fd, &buff, total_bytes);

    return 0;

}

struct hv_menu_bar *hv_client_create_menu_bar(struct hv_client *client, void *user_data)
{

    UInt32 msg_id = HV_CLIENT_CREATE_MENU_BAR_ID;
    UInt32 menu_bar_id;

    // Find a free ID
    if(hv_array_empty(client->freed_menu_bar_ids))
    {
        client->greatest_menu_bar_id++;
        menu_bar_id = client->greatest_menu_bar_id;
    }
    else
    {
        menu_bar_id = *(u_int32_t*)client->freed_menu_bar_ids->end->data;
        free(client->freed_menu_bar_ids->end->data);
        hv_array_pop_back(client->freed_menu_bar_ids);
    }

    struct hv_menu_bar *menu_bar = malloc(sizeof(struct hv_menu_bar));
    menu_bar->id = menu_bar_id;
    menu_bar->user_data = user_data;
    menu_bar->link = hv_array_push_back(client->menu_bars, menu_bar);

    /* MSG ID: UInt32
     * MENU BAR ID: UInt32 */

    int total_bytes = sizeof(UInt32)*2;
    char buff[total_bytes];

    int offset = 0;
    memcpy(&buff[offset], &msg_id, sizeof(UInt32));
    offset += sizeof(UInt32);
    memcpy(&buff[offset], &menu_bar_id, sizeof(UInt32));

    write(client->fd.fd, buff, total_bytes);

    return menu_bar;
}
