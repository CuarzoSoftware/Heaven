#include "Heaven-Server-Private.h"
#include "Heaven-Server.h"
#include <poll.h>
#include <sys/epoll.h>
#include <sys/select.h>

struct hv_server_struct
{
    int epoll_fd;
    struct pollfd fds[HV_MAX_CLIENTS+2];
    struct sockaddr_un name;
    hv_server_requests_interface *requests_interface;
    hv_client *clients[HV_MAX_CLIENTS];
};

struct hv_client_struct
{
    int fds_i;
    void *user_data;
    hv_server *server;
    hv_array *top_bars;
    hv_array *objects;
    hv_top_bar *active_top_bar;
};

struct hv_compositor_struct
{
    int a;
};

/* objects */

struct hv_object_struct
{
    UInt32 type;
    UInt32 id;
    hv_client *client;
    hv_node *link;
    struct hv_object_struct *parent;
    hv_node *parent_link;
    hv_array *children;
    void *user_data;
};

struct hv_top_bar_struct
{
    struct hv_object_struct object;
};

struct hv_menu_struct
{
    struct hv_object_struct object;
};

struct hv_action_struct
{
    struct hv_object_struct object;
};

hv_server *hv_server_create(const char *socket_name, hv_server_requests_interface *requests_interface)
{
    const char *sock_name;

    if(socket_name)
        sock_name = socket_name;
    else
        sock_name = HV_DEFAULT_SOCKET;

    hv_server *server = malloc(sizeof(hv_server));
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

int hv_server_get_fd(hv_server *server)
{
    return server->epoll_fd;
}

int hv_client_read(hv_client *client, void *dst, size_t bytes)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(client->server->fds[client->fds_i].fd, &set);

    if(select(client->server->fds[client->fds_i].fd + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        printf("Error: Client read timeout.\n");
        hv_client_destroy(client);
        return 1;
    }

    if(read(client->server->fds[client->fds_i].fd, dst, bytes) != (ssize_t)bytes)
    {
        printf("Error: Client read failed.\n");
        hv_client_destroy(client);
        return 1;
    }

    return 0;
}

hv_object *hv_client_object_get_by_id(hv_client *client, UInt32 id)
{
    hv_node *node = client->objects->begin;

    while(node)
    {
        struct hv_object_struct *object = node->data;

        if(object->id == id)
            return object;

        node = node->next;
    }

    return NULL;
}

hv_object *hv_client_object_create(hv_client *client, UInt32 type, UInt32 size)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return NULL;

    struct hv_object_struct *object = malloc(size);
    object->type = type;
    object->id = id;
    object->link = hv_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hv_array_create();
    object->user_data = NULL;

    return (hv_object*)object;
}

/* CLIENT REQUESTS HANDLERS */

void hv_client_set_app_name_handler(hv_client *client)
{
    UInt32 app_name_len;

    if(hv_client_read(client, &app_name_len, sizeof(UInt32)))
        return;

    char app_name[app_name_len+1];

    if(hv_client_read(client, app_name, app_name_len))
        return;

    app_name[app_name_len] = '\0';

    client->server->requests_interface->client_set_app_name(client, app_name);
}

void hv_top_bar_create_handler(hv_client *client)
{
    hv_top_bar *top_bar = (hv_top_bar *)hv_client_object_create(client, HV_OBJECT_TYPE_TOP_BAR, sizeof(hv_top_bar));
    if(!top_bar) return;
    top_bar->object.parent_link = hv_array_push_back(client->top_bars, top_bar);
    client->server->requests_interface->object_create(top_bar);
    if(client->active_top_bar == NULL)
    {
        client->active_top_bar = top_bar;
        client->server->requests_interface->top_bar_set_active(top_bar);
    }
}

void hv_top_bar_set_active_handler(hv_client *client)
{
    UInt32 top_bar_id;

    if(hv_client_read(client, &top_bar_id, sizeof(UInt32)))
        return;

    hv_top_bar *top_bar = (hv_top_bar *)hv_client_object_get_by_id(client, top_bar_id);

    if(!top_bar)
    {
        hv_client_destroy(client);
        return;
    }

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
    {
        hv_client_destroy(client);
        return;
    }

    if(top_bar == client->active_top_bar)
        return;

    client->active_top_bar = top_bar;
    client->server->requests_interface->top_bar_set_active(top_bar);

}

void hv_top_bar_destroy(hv_top_bar *top_bar)
{
    top_bar->object.client->server->requests_interface->object_destroy(top_bar);
    hv_array_erase(top_bar->object.client->top_bars, top_bar->object.parent_link);

    if(top_bar->object.client->active_top_bar == top_bar)
    {
        if(!hv_array_empty(top_bar->object.client->top_bars))
        {
            top_bar->object.client->active_top_bar = top_bar->object.client->top_bars->end->data;
            top_bar->object.client->server->requests_interface->top_bar_set_active(top_bar->object.client->active_top_bar);
        }
        else
        {
            top_bar->object.client->active_top_bar = NULL;
            top_bar->object.client->server->requests_interface->top_bar_set_active(NULL);
        }
    }
}

void hv_menu_create_handler(hv_client *client)
{
    hv_menu *object = (hv_menu *)hv_client_object_create(client, HV_OBJECT_TYPE_MENU, sizeof(hv_menu));
    if(!object) return;
    client->server->requests_interface->object_create(object);
}

void hv_menu_destroy(hv_menu *menu)
{
    menu->object.client->server->requests_interface->object_destroy(menu);
}

void hv_menu_set_title_handler(hv_client *client)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return;

    hv_menu *menu = (hv_menu *)hv_client_object_get_by_id(client, id);

    if(!menu)
    {
        hv_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_client_destroy(client);
        return;
    }

    UInt32 title_len;

    if(hv_client_read(client, &title_len, sizeof(UInt32)))
        return;

    if(title_len == 0)
        client->server->requests_interface->menu_set_title(menu, NULL);
    else
    {
        char title[title_len+1];

        if(hv_client_read(client, title, title_len))
            return;

        title[title_len] = '\0';

        client->server->requests_interface->menu_set_title(menu, title);
    }
}

void hv_menu_add_to_top_bar_handler(hv_client *client)
{
    UInt32 menu_id;

    if(hv_client_read(client, &menu_id, sizeof(UInt32)))
        return;

    hv_menu *menu = (hv_menu *)hv_client_object_get_by_id(client, menu_id);

    if(!menu)
    {
        hv_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_client_destroy(client);
        return;
    }

    UInt32 top_bar_id;

    if(hv_client_read(client, &top_bar_id, sizeof(UInt32)))
        return;

    hv_top_bar *top_bar = (hv_top_bar *)hv_client_object_get_by_id(client, top_bar_id);

    if(!top_bar)
    {
        hv_client_destroy(client);
        return;
    }

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
    {
        hv_client_destroy(client);
        return;
    }

    UInt32 before_id;

    if(hv_client_read(client, &before_id, sizeof(UInt32)))
        return;

    hv_menu *before = NULL;

    if(before_id == 0)
    {
        // Remove from current parent
        if(menu->object.parent)
            hv_array_erase(menu->object.parent->children, menu->object.parent_link);

        // Set parent
        menu->object.parent = (struct hv_object_struct *)top_bar;

        // Push back
        menu->object.parent_link = hv_array_push_back(menu->object.parent->children, menu);

    }
    else
    {
        before = (hv_menu *)hv_client_object_get_by_id(client, before_id);

        if(!before)
        {
            hv_client_destroy(client);
            return;
        }

        if(before->object.type != HV_OBJECT_TYPE_MENU)
        {
            hv_client_destroy(client);
            return;
        }

        if(before->object.parent != (struct hv_object_struct *)top_bar)
        {
            hv_client_destroy(client);
            return;
        }

        // Remove from current parent
        if(menu->object.parent)
            hv_array_erase(menu->object.parent->children, menu->object.parent_link);

        // Set parent
        menu->object.parent = (struct hv_object_struct *)top_bar;

        // Insert before before
        menu->object.parent_link = hv_array_insert_before(menu->object.parent->children, before->object.parent_link, menu);

    }

    client->server->requests_interface->menu_add_to_top_bar(menu, top_bar, before);

}

void hv_action_create_handler(hv_client *client)
{
    hv_action *action = (hv_action *)hv_client_object_create(client, HV_OBJECT_TYPE_ACTION, sizeof(hv_action));
    if(!action) return;
    client->server->requests_interface->object_create(action);
}

void hv_action_set_text_handler(hv_client *client)
{
    UInt32 action_id;

    if(hv_client_read(client, &action_id, sizeof(UInt32)))
        return;

    hv_action *action = (hv_action*)hv_client_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_client_destroy(client);
        return;
    }

    UInt32 text_len;

    if(hv_client_read(client, &text_len, sizeof(UInt32)))
        return;

    if(text_len == 0)
    {
        client->server->requests_interface->action_set_text(action, NULL);
        return;
    }

    char text[text_len + 1];

    if(hv_client_read(client, &text, text_len))
        return;

    text[text_len] = '\0';

    client->server->requests_interface->action_set_text(action, text);

}

void hv_object_create_handler(hv_client *client)
{
    UInt32 type;

    if(hv_client_read(client, &type, sizeof(UInt32)))
        return;

    switch(type)
    {
        case HV_OBJECT_TYPE_TOP_BAR:
        {
            hv_top_bar_create_handler(client);
            return;
        }break;
        case HV_OBJECT_TYPE_MENU:
        {
            hv_menu_create_handler(client);
            return;
        }break;
        case HV_OBJECT_TYPE_ACTION:
        {
            hv_action_create_handler(client);
            return;
        }break;
        case HV_OBJECT_TYPE_SEPARATOR:
        {

        }break;
    }

    hv_client_destroy(client);
}

void hv_object_destroy(hv_object *obj)
{
    struct hv_object_struct *object = (struct hv_object_struct *)obj;

    switch(object->type)
    {
        case HV_OBJECT_TYPE_TOP_BAR:
        {
            hv_top_bar_destroy((hv_top_bar*) object);
        }break;
        case HV_OBJECT_TYPE_MENU:
        {
            hv_menu_destroy((hv_menu*) object);
        }break;
        case HV_OBJECT_TYPE_ACTION:
        {

        }break;
        case HV_OBJECT_TYPE_SEPARATOR:
        {

        }break;
    }

    hv_array_erase(object->client->objects, object->link);

    if(object->parent)
        hv_array_erase(object->parent->children, object->parent_link);

    while(!hv_array_empty(object->children))
    {
        struct hv_object_struct *child = object->children->end->data;
        child->parent = NULL;
        child->parent_link = NULL;
        hv_array_pop_back(object->children);
    }

    hv_array_destroy(object->children);

    free(object);
}

void hv_object_destroy_handler(hv_client *client)
{
    UInt32 id;

    if(hv_client_read(client, &id, sizeof(UInt32)))
        return;

    hv_object *object = hv_client_object_get_by_id(client, id);

    if(!object)
    {
        hv_client_destroy(client);
        return;
    }

    hv_object_destroy(object);
}

void hv_client_handle_request(hv_client *client, UInt32 msg_id)
{
    switch(msg_id)
    {
        case HV_CLIENT_SET_APP_NAME_ID:
        {
            hv_client_set_app_name_handler(client);
        }break;
        case HV_TOP_BAR_SET_ACTIVE_ID:
        {
            hv_top_bar_set_active_handler(client);
        }break;
        case HV_OBJECT_CREATE_ID:
        {
            hv_object_create_handler(client);
        }break;
        case HV_OBJECT_DESTROY_ID:
        {
            hv_object_destroy_handler(client);
        }break;
        case HV_MENU_SET_TITLE_ID:
        {
            hv_menu_set_title_handler(client);
        }break;
        case HV_MENU_ADD_TO_TOP_BAR_ID:
        {
            hv_menu_add_to_top_bar_handler(client);
        }break;
        case HV_ACTION_SET_TEXT_ID:
        {
            hv_action_set_text_handler(client);
        }break;
    }
}

int hv_server_accept_connection(hv_server *server)
{
    int connection_fd = accept(server->fds[0].fd, NULL, NULL);

    if(connection_fd == -1)
        return -1;

    UInt32 type;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(connection_fd, &set);

    if(select(connection_fd + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        close(connection_fd);
        return -2;
    }

    if(read(connection_fd, &type, sizeof(UInt32)) != sizeof(UInt32))
    {
        close(connection_fd);
        return -2;
    }

    /* CREATE CLIENT */

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

        hv_client *client = malloc(sizeof(hv_client));
        client->fds_i = fds_i;
        server->clients[fds_i - 2] = client;
        client->server = server;
        client->top_bars = hv_array_create();
        client->objects = hv_array_create();
        client->active_top_bar = NULL;

        server->requests_interface->client_connected(client);
        return 0;
    }
    else
    {
        close(connection_fd);
        return -2;
    }

}

int hv_server_handle_requests(hv_server *server, int timeout)
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
                hv_client *client = server->clients[i-2];

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

                    if(hv_client_read(client, &msg_id, sizeof(UInt32)))
                        continue;

                    hv_client_handle_request(client, msg_id);
                }
            }
        }
    }

    return 0;
}

void hv_client_destroy(hv_client *client)
{
    if(client->active_top_bar)
    {
        client->active_top_bar = NULL;
        client->server->requests_interface->top_bar_set_active(NULL);
    }

    epoll_ctl(client->server->epoll_fd, EPOLL_CTL_DEL, client->server->fds[client->fds_i].fd, NULL);
    close(client->server->fds[client->fds_i].fd);
    client->server->fds[client->fds_i].fd = -1;

    while(!hv_array_empty(client->objects))
        hv_object_destroy(client->objects->end->data);

    hv_array_destroy(client->top_bars);

    hv_array_destroy(client->objects);

    client->server->requests_interface->client_disconnected(client);

    free(client);
}

