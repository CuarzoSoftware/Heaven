#include "Heaven-Server-Private.h"
#include "Heaven-Server.h"
#include <poll.h>
#include <sys/epoll.h>

struct hv_server
{
    int epoll_fd;
    struct pollfd fds[HV_MAX_CLIENTS+2];
    struct sockaddr_un name;
    struct hv_server_requests_interface *requests_interface;
    struct hv_client *clients[HV_MAX_CLIENTS];
};

struct hv_client
{
    int fds_i;
    void *user_data;
    struct hv_server *server;
    struct hv_array *menu_bars;
};

struct hv_compositor
{
    int a;
};

struct hv_menu_bar
{
    int id;
    void *user_data;
    struct hv_client *client;
    struct hv_node *link;
    struct hv_array *menus;
};

struct hv_server *hv_create_server(const char *socket_name, struct hv_server_requests_interface *requests_interface)
{
    const char *sock_name;

    if(socket_name)
        sock_name = socket_name;
    else
        sock_name = HV_DEFAULT_SOCKET;

    struct hv_server *server = malloc(sizeof(struct hv_server));
    server->requests_interface = requests_interface;
    server->fds[0].events = POLLIN;
    server->fds[0].revents = 0;

    for(int i = 1; i < HV_MAX_CLIENTS+2; i++)
    {
        server->fds[i].fd = -1;
        server->fds[i].events = POLLIN | POLLHUP;
        server->fds[i].revents = 0;
    }

    unlink(sock_name);

    if( (server->fds[0].fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(server);
        return NULL;
    }

    memset(&server->name.sun_path, 0, 108);
    server->name.sun_family = AF_UNIX;
    strncpy(server->name.sun_path, sock_name, 107);

    if(bind(server->fds[0].fd, (const struct sockaddr*)&server->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: Could not bind socket.\n");
        free(server);
        return NULL;
    }

    if(listen(server->fds[0].fd, HV_MAX_CLIENTS+1) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(server);
        return NULL;
    }

    struct epoll_event epoll_ev;
    epoll_ev.events = POLLIN;
    server->epoll_fd = epoll_create(HV_MAX_CLIENTS+2);
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->fds[0].fd, &epoll_ev);

    return server;
}

int hv_server_get_fd(struct hv_server *server)
{
    return server->epoll_fd;
}

/* CLIENT REQUESTS */

int hv_client_read(struct hv_client *client, void *dst, size_t bytes)
{
    if(read(client->server->fds[client->fds_i].fd, dst, bytes) < 0)
    {
        hv_client_destroy(client->server->clients[client->fds_i-2]);
        return 1;
    }

    return 0;
}

void hv_client_set_app_name_handler(struct hv_client *client)
{
    u_int32_t app_name_len;

    if(hv_client_read(client, &app_name_len, sizeof(app_name_len)))
        return;

    char app_name[app_name_len+1];

    if(hv_client_read(client, app_name, app_name_len))
        return;

    app_name[app_name_len] = '\0';

    client->server->requests_interface->client_set_app_name_request(client, app_name);
}

void hv_client_create_menu_bar_handler(struct hv_client *client)
{
    UInt32 menu_bar_id;

    if(hv_client_read(client, &menu_bar_id, sizeof(UInt32)))
        return;

    struct hv_menu_bar *menu_bar = malloc(sizeof(struct hv_menu_bar));
    menu_bar->menus = hv_array_create();
    menu_bar->client = client;
    menu_bar->id = menu_bar_id;
    menu_bar->link = hv_array_push_back(client->menu_bars, menu_bar);

    client->server->requests_interface->client_create_menu_bar_request(client, menu_bar);
}

void hv_client_handle_request(struct hv_client *client, u_int32_t msg_id)
{
    switch(msg_id)
    {
        case HV_CLIENT_SET_APP_NAME_ID:
        {
            hv_client_set_app_name_handler(client);
        }break;
        case HV_CLIENT_CREATE_MENU_BAR_ID:
        {
            hv_client_create_menu_bar_handler(client);
        }break;
    }
}

int hv_server_accept_client(struct hv_server *server)
{
    int client_fd = accept(server->fds[0].fd, NULL, NULL);

    if(client_fd == -1)
        return -1;

    int fds_i = hv_fds_add_fd(server->fds, client_fd);

    if(fds_i == -1)
    {
        close(client_fd);
        printf("Error: Clients limit reached.\n");
        return -2;
    }

    struct epoll_event epoll_ev;
    epoll_ev.events = POLLIN | POLLHUP;
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &epoll_ev);

    struct hv_client *client = malloc(sizeof(struct hv_client));
    client->fds_i = fds_i;
    server->clients[fds_i - 2] = client;
    client->server = server;
    client->menu_bars = hv_array_create();

    server->requests_interface->client_connected_request(client);
    return 0;
}

int hv_server_handle_requests(struct hv_server *server, int timeout)
{
    int ret = poll(server->fds, HV_MAX_CLIENTS+2, timeout);

    if(ret == -1)
        return -1;

    if(ret != 0)
    {
        // New client
        if(server->fds[0].revents & POLLIN)
            hv_server_accept_client(server);


        for(int i = 2; i < HV_MAX_CLIENTS+2; i++)
        {
            if(server->fds[i].fd != -1)
            {
                struct hv_client *client = server->clients[i-2];

                // Client disconnected
                if(server->fds[i].revents & POLLHUP)
                {
                    hv_client_destroy(client);
                    continue;
                }

                // Client request
                if(server->fds[i].revents & POLLIN)
                {
                    u_int32_t msg_id;

                    if(hv_client_read(client, &msg_id, sizeof(u_int32_t)))
                        continue;

                    hv_client_handle_request(client, msg_id);
                }
            }
        }
    }

    return 0;
}

void hv_client_destroy(struct hv_client *client)
{
    client->server->requests_interface->client_disconnected_request(client);
    epoll_ctl(client->server->epoll_fd, EPOLL_CTL_DEL, client->server->fds[client->fds_i].fd, NULL);
    close(client->server->fds[client->fds_i].fd);
    client->server->fds[client->fds_i].fd = -1;
    free(client);
}





/* MENU BAR */





UInt32 hv_menu_bar_get_id(struct hv_menu_bar *menu_bar)
{
    return menu_bar->id;
}

