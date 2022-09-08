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
    struct hv_array *objects;
};

struct hv_compositor
{
    int a;
};

/* objects */

struct hv_object
{
    UInt32 type;
    UInt32 id;
    struct hv_client *client;
    struct hv_node *link;
    struct hv_object *parent;
    struct hv_node *parent_link;
    struct hv_array *children;
    void *user_data;
};

struct hv_menu_bar
{
    struct hv_object object;
};

struct hv_menu
{
    struct hv_object object;
};

struct hv_server *hv_server_create(const char *socket_name, struct hv_server_requests_interface *requests_interface)
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

int hv_client_read(struct hv_client *client, void *dst, size_t bytes)
{
    if(read(client->server->fds[client->fds_i].fd, dst, bytes) <= 0)
    {
        printf("READ FAILED\n");
        hv_client_destroy(client);
        return 1;
    }

    return 0;
}

struct hv_object *hv_client_object_get_by_id(struct hv_client *client, UInt32 id)
{
    struct hv_node *node = client->objects->begin;

    while(node)
    {
        struct hv_object *object = node->data;

        if(object->id == id)
            return object;

        node = node->next;
    }

    return NULL;
}

struct hv_object *hv_client_object_create(struct hv_client *client, UInt32 type, UInt32 size)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return NULL;

    struct hv_object *object = malloc(size);
    object->type = type;
    object->id = id;
    object->link = hv_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hv_array_create();
    object->user_data = NULL;

    return object;
}

void hv_object_destroy(struct hv_object *object)
{
    hv_array_erase(object->client->objects, object->link);

    if(object->parent)
        hv_array_erase(object->parent->children, object->parent_link);

    while(!hv_array_empty(object->children))
    {
        struct hv_object *child = object->children->end->data;
        child->parent = NULL;
        child->parent_link = NULL;
        hv_array_pop_back(object->children);
    }

    hv_array_destroy(object->children);

    free(object);
}

struct hv_object *hv_object_destroy_handler(struct hv_client *client)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return NULL;

    struct hv_node *node = client->objects->begin;

    while(node)
    {
        struct hv_object *object = node->data;

        if(object->id == id)
            return object;

        node = node->next;
    }

    // IF menu_bar doesn't exist
    hv_client_destroy(client);

    return NULL;
}

/* CLIENT REQUESTS HANDLERS */

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

void hv_menu_bar_create_handler(struct hv_client *client)
{
    struct hv_menu_bar *object = (struct hv_menu_bar *)hv_client_object_create(client, HV_OBJECT_TYPE_MENU_BAR, sizeof(struct hv_menu_bar));
    if(!object) return;
    object->object.parent_link = hv_array_push_back(client->menu_bars, object);
    client->server->requests_interface->menu_bar_create_request(object);
}

void hv_menu_bar_destroy(struct hv_menu_bar *object)
{
    object->object.client->server->requests_interface->menu_bar_destroy_request(object);
    hv_array_erase(object->object.client->objects, object->object.link);
    hv_array_erase(object->object.client->menu_bars, object->object.parent_link);
    hv_array_destroy(object->object.children);
    free(object);
}

void hv_menu_bar_destroy_handler(struct hv_client *client)
{
    struct hv_menu_bar *object = (struct hv_menu_bar *)hv_object_destroy_handler(client);

    if(!object)
        return;

    hv_menu_bar_destroy(object);
}

void hv_menu_create_handler(struct hv_client *client)
{
    struct hv_menu *object = (struct hv_menu *)hv_client_object_create(client, HV_OBJECT_TYPE_MENU, sizeof(struct hv_menu));
    if(!object) return;
    client->server->requests_interface->menu_create_request(object);
}

void hv_menu_destroy(struct hv_menu *object)
{
    object->object.client->server->requests_interface->menu_destroy_request(object);
    hv_object_destroy((struct hv_object *)object);
}

void hv_menu_destroy_handler(struct hv_client *client)
{
    struct hv_menu *object = (struct hv_menu *)hv_object_destroy_handler(client);

    if(!object)
        return;

    hv_menu_destroy(object);
}

void hv_menu_set_title_handler(struct hv_client *client)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return;

    struct hv_menu *object = (struct hv_menu *)hv_client_object_get_by_id(client, id);

    if(!object)
    {
        hv_client_destroy(client);
        return;
    }

    if(object->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_client_destroy(client);
        return;
    }

    UInt32 title_len;

    if(hv_client_read(client, &title_len, sizeof(UInt32)))
        return;

    if(title_len == 0)
        client->server->requests_interface->menu_set_title_request(object, NULL);
    else
    {
        char title[title_len+1];

        if(hv_client_read(client, title, title_len))
            return;

        title[title_len] = '\0';

        client->server->requests_interface->menu_set_title_request(object, title);
    }


}

void hv_client_handle_request(struct hv_client *client, UInt32 msg_id)
{
    switch(msg_id)
    {
        case HV_CLIENT_SET_APP_NAME_ID:
        {
            hv_client_set_app_name_handler(client);
        }break;
        case HV_MENU_BAR_CREATE_ID:
        {
            hv_menu_bar_create_handler(client);
        }break;
        case HV_MENU_BAR_DESTROY_ID:
        {
            hv_menu_bar_destroy_handler(client);
        }break;
        case HV_MENU_CREATE_ID:
        {
            hv_menu_create_handler(client);
        }break;
        case HV_MENU_SET_TITLE_ID:
        {
            hv_menu_set_title_handler(client);
        }break;
        case HV_MENU_DESTROY_ID:
        {
            hv_menu_destroy_handler(client);
        }break;
    }
}

int hv_server_accept_connection(struct hv_server *server)
{
    int connection_fd = accept(server->fds[0].fd, NULL, NULL);

    if(connection_fd == -1)
        return -1;

    UInt32 msg_id;

    if(read(connection_fd, &msg_id, sizeof(UInt32)) <= 0)
    {
        close(connection_fd);
        return -2;
    }

    if(msg_id != HV_COMMON_IDENTIFY_ID)
    {
        close(connection_fd);
        return -2;
    }

    UInt32 type;

    if(read(connection_fd, &type, sizeof(UInt32)) <= 0)
    {
        close(connection_fd);
        return -2;
    }

    if(type == HV_CONNECTION_TYPE_CLIENT)
    {
        int fds_i = hv_fds_add_fd(server->fds, connection_fd);

        if(fds_i == -1)
        {
            close(connection_fd);
            printf("Error: Clients limit reached.\n");
            return -2;
        }

        struct epoll_event epoll_ev;
        epoll_ev.events = POLLIN | POLLHUP;
        epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, connection_fd, &epoll_ev);

        struct hv_client *client = malloc(sizeof(struct hv_client));
        client->fds_i = fds_i;
        server->clients[fds_i - 2] = client;
        client->server = server;
        client->menu_bars = hv_array_create();
        client->objects = hv_array_create();

        server->requests_interface->client_connected(client);
        return 0;
    }
    else
    {
        close(connection_fd);
        return -2;
    }

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
            hv_server_accept_connection(server);


        for(int i = 2; i < HV_MAX_CLIENTS+2; i++)
        {
            if(server->fds[i].fd != -1)
            {
                struct hv_client *client = server->clients[i-2];

                // Client disconnected
                if(server->fds[i].revents & POLLHUP)
                {
                    printf("POLLHUP\n");
                    hv_client_destroy(client);
                    continue;
                }

                // Client request
                if(server->fds[i].revents & POLLIN)
                {
                    u_int32_t msg_id;

                    if(hv_client_read(client, &msg_id, sizeof(UInt32)))
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
    epoll_ctl(client->server->epoll_fd, EPOLL_CTL_DEL, client->server->fds[client->fds_i].fd, NULL);
    close(client->server->fds[client->fds_i].fd);
    client->server->fds[client->fds_i].fd = -1;

    while(!hv_array_empty(client->objects))
    {
        UInt32 *type = client->objects->end->data;

        switch(*type)
        {
            case HV_OBJECT_TYPE_MENU_BAR:
            {
                hv_menu_bar_destroy(client->objects->end->data);
            }break;
            case HV_OBJECT_TYPE_MENU:
            {
                hv_menu_destroy(client->objects->end->data);
            }break;
            case HV_OBJECT_TYPE_ITEM:
            {

            }break;
            case HV_OBJECT_TYPE_SEPARATOR:
            {

            }break;
        }
    }

    hv_array_destroy(client->menu_bars);

    hv_array_destroy(client->objects);

    client->server->requests_interface->client_disconnected(client);

    free(client);
}

