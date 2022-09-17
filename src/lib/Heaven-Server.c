#include "Heaven-Server.h"
#include <asm-generic/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <pthread.h>

struct hn_server_struct
{
    int epoll_fd;
    struct pollfd fds[HN_MAX_CLIENTS+2];
    struct sockaddr_un name;
    hn_server_requests_interface *requests_interface;
    hn_client *clients[HN_MAX_CLIENTS];
    void *user_data;
    pthread_mutex_t mutex;
    hn_compositor *compositor;
    hn_client *active_client;
};

struct hn_creds
{
    pid_t pid;    /* process ID of the sending process */
    uid_t uid;    /* user ID of the sending process */
    gid_t gid;    /* group ID of the sending process */
};

struct hn_client_struct
{
    hn_array *objects;
    void *user_data;
    int fds_i;
    hn_server *server;
    hn_array *top_bars;
    hn_top_bar *active_top_bar;
    struct hn_creds creds;
};

struct hn_compositor_struct
{
    void *user_data;
    hn_server *server;
};

/* objects */

struct hn_object_struct
{
    hn_object_type type;
    hn_object_id id;
    hn_client *client;
    struct hn_object_struct *parent;
    hn_node *link;
    hn_node *parent_link;
    hn_array *children;
    void *user_data;
    hn_bool enabled;
    hn_bool checked;
    hn_bool active;
};

struct hn_top_bar_struct
{
    struct hn_object_struct object;
};

struct hn_menu_struct
{
    struct hn_object_struct object;
};

struct hn_action_struct
{
    struct hn_object_struct object;
};

struct hn_select_struct
{
    struct hn_object_struct object;
    hn_option *active_option;
};

struct hn_option_struct
{
    struct hn_object_struct object;
};

struct hn_toggle_struct
{
    struct hn_object_struct object;
};

struct hn_separator_struct
{
    struct hn_object_struct object;
};

/* UTILS */

int hn_fds_add_fd(struct pollfd *fds, int fd)
{
    for(int i = 2; i < HN_MAX_CLIENTS+2; i++)
    {
        if(fds[i].fd == -1)
        {
            fds[i].fd = fd;
            return i;
        }
    }

    return -1;
}

int hn_client_read(hn_client *client, void *dst, ssize_t bytes)
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
        hn_server_client_destroy(client);
        return 1;
    }

    u_int8_t *data = dst;

    ssize_t readBytes = read(client->server->fds[client->fds_i].fd, data, bytes);

    if(readBytes < 1)
    {
        printf("Error: Client read failed.\n");
        hn_server_client_destroy(client);
        return 1;
    }

    else if(readBytes < bytes)
        return hn_client_read(client, &data[readBytes], bytes - readBytes);

    return 0;
}

int hn_compositor_read(hn_compositor *compositor, void *dst, ssize_t bytes)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(compositor->server->fds[1].fd, &set);

    if(select(compositor->server->fds[1].fd + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        printf("Error: Compositor read timeout.\n");
        hn_server_compositor_destroy(compositor);
        return 1;
    }

    u_int8_t *data = dst;

    ssize_t readBytes = read(compositor->server->fds[1].fd, data, bytes);

    if(readBytes < 1)
    {
        printf("Error: Compositor read failed.\n");
        hn_server_compositor_destroy(compositor);
        return 1;
    }

    else if(readBytes < bytes)
        return hn_compositor_read(compositor, &data[readBytes], bytes - readBytes);

    return 0;
}

int hn_client_write(hn_client *client, void *src, ssize_t bytes)
{
    u_int8_t *data = src;

    ssize_t writtenBytes = write(client->server->fds[client->fds_i].fd, data, bytes);

    if(writtenBytes < 1)
    {
        hn_server_client_destroy(client);
        return 1;
    }
    else if(writtenBytes < bytes)
        return hn_client_write(client, &data[writtenBytes], bytes - writtenBytes);

    return 0;
}

int hn_compositor_write(hn_compositor *compositor, void *src, ssize_t bytes)
{
    u_int8_t *data = src;

    ssize_t writtenBytes = write(compositor->server->fds[1].fd, data, bytes);

    if(writtenBytes < 1)
    {
        hn_server_compositor_destroy(compositor);
        return 1;
    }
    else if(writtenBytes < bytes)
        return hn_compositor_write(compositor, &data[writtenBytes], bytes - writtenBytes);

    return 0;
}

/* SERVER */

hn_server *hn_server_create(const char *socket_name, void *user_data, hn_server_requests_interface *requests_interface)
{
    const char *sock_name;

    if(socket_name)
    {
        if(socket_name[0] == '/')
        {
            printf("Error: Invalid socket name \"%s\". Can not start with \"/\".\n", socket_name);
            return NULL;
        }
        sock_name = socket_name;
    }
    else
        sock_name = HN_DEFAULT_SOCKET;

    char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    if(!xdg_runtime_dir)
    {
        printf("Error: XDG_RUNTIME_DIR env not set.\n");
        return NULL;
    }

    hn_server *server = malloc(sizeof(hn_server));
    server->requests_interface = requests_interface;
    server->fds[0].events = POLLIN;
    server->fds[0].revents = 0;
    server->user_data = user_data;
    server->compositor = NULL;
    server->active_client = NULL;

    if(pthread_mutex_init(&server->mutex, NULL) != 0)
    {
        printf("Error: Could not create mutex.\n");
        free(server);
        return NULL;
    }

    for(int i = 1; i < HN_MAX_CLIENTS+2; i++)
    {
        server->fds[i].fd = -1;
        server->fds[i].events = POLLIN | POLLHUP;
        server->fds[i].revents = 0;
    }

    if( (server->fds[0].fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(server);
        return NULL;
    }

    int xdg_runtime_dir_len = strlen(xdg_runtime_dir);

    memset(&server->name.sun_path, 0, 108);
    server->name.sun_family = AF_UNIX;
    memcpy(server->name.sun_path, xdg_runtime_dir, xdg_runtime_dir_len);

    if(xdg_runtime_dir[xdg_runtime_dir_len-1] != '/')
    {
        server->name.sun_path[xdg_runtime_dir_len] = '/';
        xdg_runtime_dir_len++;
    }

    strncpy(&server->name.sun_path[xdg_runtime_dir_len], sock_name, 107 - xdg_runtime_dir_len);

    unlink(server->name.sun_path);

    if(bind(server->fds[0].fd, (const struct sockaddr*)&server->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: Could not bind socket.\n");
        free(server);
        return NULL;
    }

    if(listen(server->fds[0].fd, HN_MAX_CLIENTS+1) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(server);
        return NULL;
    }

    struct epoll_event epoll_ev;
    epoll_ev.events = POLLIN;
    server->epoll_fd = epoll_create(HN_MAX_CLIENTS+2);
    epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->fds[0].fd, &epoll_ev);

    return server;
}

int hn_server_get_fd(hn_server *server)
{
    return server->epoll_fd;
}

void *hn_server_get_user_data(hn_server *server)
{
    return server->user_data;
}

void hn_server_set_user_data(hn_server *server, void *user_data)
{
    server->user_data = user_data;
}

hn_compositor *hn_server_get_compositor(hn_server *server)
{
    return server->compositor;
}

/* CLIENT */

void hn_client_get_credentials(hn_client *client, pid_t *pid, uid_t *uid, gid_t *gid)
{
    if(pid)
        *pid = client->creds.pid;
    if(uid)
        *uid = client->creds.uid;
    if(gid)
        *gid = client->creds.gid;
}

hn_client *hn_client_get_by_pid(hn_server *server, hn_client_pid pid)
{
    for(int i = 2; i < HN_MAX_CLIENTS; i++)
    {
        if(server->fds[i].fd != -1)
        {
            if(server->clients[i-2]->creds.pid == pid)
            {
                return server->clients[i-2];
            }
        }
    }

    return NULL;
}

hn_client_id hn_client_get_id(hn_client *client)
{
    return client->fds_i-1;
}

hn_server *hn_client_get_server(hn_client *client)
{
    return client->server;
}

hn_object *hn_client_object_create(hn_client *client, hn_object_type type, u_int32_t size)
{
    hn_object_id id;

    if(hn_client_read(client, &id, sizeof(hn_object_id)))
        return NULL;

    struct hn_object_struct *object = malloc(size);
    object->type = type;
    object->id = id;
    object->link = hn_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hn_array_create();
    object->user_data = NULL;
    object->enabled = HN_TRUE;
    object->checked = HN_FALSE;
    object->active = HN_FALSE;

    return (hn_object*)object;
}

void hn_object_remove_from_parent(struct hn_object_struct *object)
{
    if(!object)
        return;

    if(!object->parent)
        return;

    object->active = HN_FALSE;

    object->client->server->requests_interface->client_object_set_parent(object->client, object, NULL, NULL);

    hn_array_erase(object->parent->children, object->parent_link);

    object->parent_link = NULL;
    object->parent = NULL;
}

void hn_client_set_app_name_handler(hn_client *client)
{
    hn_string_length app_name_len;

    if(hn_client_read(client, &app_name_len, sizeof(hn_string_length)))
        return;

    char app_name[app_name_len+1];

    if(hn_client_read(client, app_name, app_name_len))
        return;

    app_name[app_name_len] = '\0';

    client->server->requests_interface->client_set_app_name(client, app_name);
}

void hn_client_send_custom_request_handler(hn_client *client)
{
    hn_string_length data_len;

    if(hn_client_read(client, &data_len, sizeof(u_int32_t)))
        return;

    if(data_len == 0)
        return;

    u_int8_t data[data_len];

    if(hn_client_read(client, data, data_len))
        return;

    client->server->requests_interface->client_send_custom_request(client, data, data_len);
}

/* COMPOSITOR */

hn_server *hn_compositor_get_server(hn_compositor *compositor)
{
    return compositor->server;
}

/* TOP BAR */

void hn_top_bar_create_handler(hn_client *client)
{
    hn_top_bar *top_bar = (hn_top_bar *)hn_client_object_create(client, HN_OBJECT_TYPE_TOP_BAR, sizeof(hn_top_bar));
    if(!top_bar) return;
    top_bar->object.parent_link = hn_array_push_back(client->top_bars, top_bar);
    client->server->requests_interface->client_object_create(client, top_bar);
    if(client->active_top_bar == NULL)
    {
        top_bar->object.active = HN_TRUE;
        client->active_top_bar = top_bar;
        client->server->requests_interface->client_object_set_active(client, top_bar);
    }
}

void hn_top_bar_before_destroy(hn_top_bar *top_bar)
{
    hn_client *client = hn_object_get_client(top_bar);
    hn_array_erase(top_bar->object.client->top_bars, top_bar->object.parent_link);

    if(top_bar->object.client->active_top_bar == top_bar)
    {
        if(!hn_array_empty(top_bar->object.client->top_bars))
        {
            top_bar->object.client->active_top_bar = top_bar->object.client->top_bars->end->data;
            top_bar->object.client->active_top_bar->object.active = HN_TRUE;
            top_bar->object.client->server->requests_interface->client_object_set_active(client, top_bar->object.client->active_top_bar);
        }
        else
            top_bar->object.client->active_top_bar = NULL; 
    }
}

/* MENU */

void hn_menu_create_handler(hn_client *client)
{
    hn_menu *object = (hn_menu *)hn_client_object_create(client, HN_OBJECT_TYPE_MENU, sizeof(hn_menu));
    if(!object) return;
    client->server->requests_interface->client_object_create(client, object);
}

void hn_menu_before_destroy(hn_menu *menu)
{
    HN_UNUSED(menu);
}

/* ACTION */

void hn_action_create_handler(hn_client *client)
{
    hn_action *action = (hn_action *)hn_client_object_create(client, HN_OBJECT_TYPE_ACTION, sizeof(hn_action));
    if(!action) return;
    client->server->requests_interface->client_object_create(client, action);
}

void hn_action_before_destroy(hn_action *action)
{
    HN_UNUSED(action);
}

/* TOGGLE */

void hn_toggle_create_handler(hn_client *client)
{
    hn_toggle *toggle = (hn_toggle *)hn_client_object_create(client, HN_OBJECT_TYPE_TOGGLE, sizeof(hn_toggle));
    if(!toggle) return;
    client->server->requests_interface->client_object_create(client, toggle);
    client->server->requests_interface->client_object_set_checked(client, toggle, HN_FALSE);
}

void hn_toggle_before_destroy(hn_toggle *toggle)
{
    HN_UNUSED(toggle);
}

/* SELECT */

void hn_select_create_handler(hn_client *client)
{
    hn_select *select = (hn_select *)hn_client_object_create(client, HN_OBJECT_TYPE_SELECT, sizeof(hn_select));
    if(!select) return;
    select->active_option = NULL;
    client->server->requests_interface->client_object_create(client, select);
}

void hn_select_before_destroy(hn_select *select)
{
    HN_UNUSED(select);
}

/* OPTION */

void hn_option_create_handler(hn_client *client)
{
    hn_option *option = (hn_option *)hn_client_object_create(client, HN_OBJECT_TYPE_OPTION, sizeof(hn_option));
    if(!option) return;
    client->server->requests_interface->client_object_create(client, option);
}

void hn_option_before_destroy(hn_option *option)
{
    HN_UNUSED(option);
}


/* SEPARATOR */

void hn_separator_create_handler(hn_client *client)
{
    hn_separator *separator = (hn_separator *)hn_client_object_create(client, HN_OBJECT_TYPE_SEPARATOR, sizeof(hn_separator));
    if(!separator) return;
    client->server->requests_interface->client_object_create(client, separator);
}

void hn_separator_before_destroy(hn_separator *separator)
{
    HN_UNUSED(separator);
}


/* OBJECT */

hn_object *hn_object_get_user_data(hn_object *object)
{
    struct hn_object_struct *obj = object;
    return obj->user_data;
}

void hn_object_set_user_data(hn_object *object, void *user_data)
{
    struct hn_object_struct *obj = object;
    obj->user_data = user_data;
}

void hn_object_create_handler(hn_client *client)
{
    hn_object_type type;

    if(hn_client_read(client, &type, sizeof(hn_object_type)))
        return;

    switch(type)
    {
        case HN_OBJECT_TYPE_TOP_BAR:
        {
            hn_top_bar_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_MENU:
        {
            hn_menu_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_ACTION:
        {
            hn_action_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_TOGGLE:
        {
            hn_toggle_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_SELECT:
        {
            hn_select_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_OPTION:
        {
            hn_option_create_handler(client);
            return;
        }break;
        case HN_OBJECT_TYPE_SEPARATOR:
        {
            hn_separator_create_handler(client);
            return;
        }break;
    }

    hn_server_client_destroy(client);
}

void hn_object_set_parent_handler(hn_client *client)
{
    hn_object_id id;

    if(hn_client_read(client, &id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_object_id parent_id;

    if(hn_client_read(client, &parent_id, sizeof(hn_object_id)))
        return;

    // Remove parent
    if(parent_id == 0)
    {
        if(object->parent == NULL)
        {
            hn_server_client_destroy(client);
            return;
        }

        switch(object->type)
        {
            case HN_OBJECT_TYPE_MENU:
            {
                hn_object_remove_from_parent(object);
                return;
            }break;
            case HN_OBJECT_TYPE_ACTION:
            {
                hn_object_remove_from_parent(object);
                return;
            }break;
            case HN_OBJECT_TYPE_TOGGLE:
            {
                hn_object_remove_from_parent(object);
                return;
            }break;
            case HN_OBJECT_TYPE_SEPARATOR:
            {
                hn_object_remove_from_parent(object);
                return;
            }break;
            case HN_OBJECT_TYPE_SELECT:
            {
                hn_object_remove_from_parent(object);
                return;
            }break;
            case HN_OBJECT_TYPE_OPTION:
            {
                hn_select *parent = (hn_select *)object->parent;

                if((hn_option*)object == parent->active_option)
                {
                    hn_array_erase(parent->object.children, object->parent_link);

                    if(hn_array_empty(parent->object.children))
                    {
                        parent->active_option = NULL;
                        object->active = HN_FALSE;
                        object->parent = NULL;
                        object->parent_link = NULL;
                        client->server->requests_interface->client_object_set_parent(client, object, NULL, NULL);
                        return;
                    }
                    else
                    {
                        hn_node *node = parent->object.children->end;
                        hn_option *option;

                        while(node)
                        {
                            option = node->data;

                            if(option->object.enabled)
                            {
                                parent->active_option = option;
                                option->object.active = HN_TRUE;
                                object->active = HN_FALSE;
                                object->parent = NULL;
                                object->parent_link = NULL;
                                client->server->requests_interface->client_object_set_active(client, option);
                                client->server->requests_interface->client_object_set_parent(client, object, NULL, NULL);
                                return;
                            }

                            node = node->prev;
                        }
                        parent->active_option->object.active = HN_FALSE;
                        parent->active_option = NULL;
                        object->active = HN_FALSE;
                        object->parent = NULL;
                        object->parent_link = NULL;
                        client->server->requests_interface->client_object_set_parent(client, object, NULL, NULL);
                        return;
                    }
                }
                else
                {
                    hn_object_remove_from_parent(object);
                    return;
                }

            }break;
        }

        hn_server_client_destroy(client);
        return;
    }
    // Set parent
    else
    {
        struct hn_object_struct *parent = hn_object_get_by_id(client, parent_id);

        if(!parent)
        {
            hn_server_client_destroy(client);
            return;
        }

        if(parent == object)
        {
            hn_server_client_destroy(client);
            return;
        }



        switch(object->type)
        {
            case HN_OBJECT_TYPE_MENU:
            {
                if(parent->type != HN_OBJECT_TYPE_MENU && parent->type != HN_OBJECT_TYPE_TOP_BAR)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
            case HN_OBJECT_TYPE_ACTION:
            {
                if(parent->type != HN_OBJECT_TYPE_MENU)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
            case HN_OBJECT_TYPE_TOGGLE:
            {
                if(parent->type != HN_OBJECT_TYPE_MENU)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
            case HN_OBJECT_TYPE_SEPARATOR:
            {
                if(parent->type != HN_OBJECT_TYPE_MENU && parent->type != HN_OBJECT_TYPE_SELECT)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
            case HN_OBJECT_TYPE_SELECT:
            {
                if(parent->type != HN_OBJECT_TYPE_MENU)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
            case HN_OBJECT_TYPE_OPTION:
            {
                if(parent->type != HN_OBJECT_TYPE_SELECT)
                {
                    hn_server_client_destroy(client);
                    return;
                }
            }break;
        }

        hn_object_id before_id;

        if(hn_client_read(client, &before_id, sizeof(hn_object_id)))
            return;

        // Insert back
        if(before_id == 0)
        {
            hn_object_remove_from_parent(object);
            object->parent = parent;
            object->parent_link = hn_array_push_back(parent->children, object);
            client->server->requests_interface->client_object_set_parent(client, object, parent, NULL);
        }
        // Insert before
        else
        {
            struct hn_object_struct *before = hn_object_get_by_id(client, before_id);

            if(!before)
            {
                hn_server_client_destroy(client);
                return;
            }

            if(before->parent != parent)
            {
                hn_server_client_destroy(client);
                return;
            }

            hn_object_remove_from_parent(object);
            object->parent = parent;
            object->parent_link = hn_array_insert_before(parent->children, before->parent_link, object);
            client->server->requests_interface->client_object_set_parent(client, object, parent, before);
        }

        if(object->type == HN_OBJECT_TYPE_OPTION)
        {
            hn_select *sel = (hn_select*)parent;

            if(sel->active_option == NULL && object->enabled)
            {
                object->active = HN_TRUE;
                sel->active_option = (hn_option*)object;
                client->server->requests_interface->client_object_set_active(client, object);
            }
        }
    }



}

void hn_object_set_label_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type != HN_OBJECT_TYPE_MENU &&
            object->type != HN_OBJECT_TYPE_SEPARATOR &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_string_length label_len;

    if(hn_client_read(client, &label_len, sizeof(hn_string_length)))
        return;

    if(label_len == 0)
    {
        client->server->requests_interface->client_object_set_label(client, object, NULL);
        return;
    }

    char label[label_len + 1];

    if(hn_client_read(client, &label, label_len))
        return;

    label[label_len] = '\0';

    client->server->requests_interface->client_object_set_label(client, object, label);
}

void hn_object_set_icon_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type != HN_OBJECT_TYPE_MENU && object->type != HN_OBJECT_TYPE_ACTION)
    {
        hn_server_client_destroy(client);
        return;
    }

    u_int32_t width;

    if(hn_client_read(client, &width, sizeof(u_int32_t)))
        return;

    if(width == 0)
    {
        client->server->requests_interface->client_object_set_icon(client, object, NULL, 0, 0);
        return;
    }

    u_int32_t height;

    if(hn_client_read(client, &height, sizeof(u_int32_t)))
        return;

    if(height == 0)
    {
        hn_server_client_destroy(client);
        return;
    }

    u_int32_t total_pixels = width*height;

    hn_pixel pixels[total_pixels];

    if(hn_client_read(client, pixels, total_pixels))
        return;

    client->server->requests_interface->client_object_set_icon(client, object, pixels, width, height);
}

void hn_object_set_shortcuts_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type != HN_OBJECT_TYPE_SEPARATOR &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_string_length shortcuts_len;

    if(hn_client_read(client, &shortcuts_len, sizeof(hn_string_length)))
        return;

    if(shortcuts_len == 0)
    {
        client->server->requests_interface->client_object_set_shortcuts(client, object, NULL);
        return;
    }

    char shortcuts[shortcuts_len+1];

    if(hn_client_read(client, shortcuts, shortcuts_len))
        return;

    shortcuts[shortcuts_len] = '\0';

    client->server->requests_interface->client_object_set_shortcuts(client, object, shortcuts);
}

void hn_object_set_checked_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type != HN_OBJECT_TYPE_TOGGLE)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_bool checked;

    if(hn_client_read(client, &checked, sizeof(hn_bool)))
        return;

    if(checked != HN_TRUE && checked != HN_FALSE)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->checked == checked)
    {
        hn_server_client_destroy(client);
        return;
    }

    object->checked = checked;

    client->server->requests_interface->client_object_set_checked(client, object, checked);
}

void hn_object_set_active_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        printf("Active object not found\n");
        hn_server_client_destroy(client);
        return;
    }

    if(object->active == HN_TRUE)
    {
        printf("Object already activen");
        hn_server_client_destroy(client);
        return;
    }

    if(object->type == HN_OBJECT_TYPE_OPTION)
    {
        if(!object->parent)
        {
            hn_server_client_destroy(client);
            return;
        }

        hn_select *sel = (hn_select*)object->parent;

        if(sel->active_option)
            sel->active_option->object.active = HN_FALSE;

        sel->active_option = (hn_option*)object;
    }
    else if(object->type == HN_OBJECT_TYPE_TOP_BAR)
    {
        if(client->active_top_bar)
            client->active_top_bar->object.active = HN_FALSE;

        client->active_top_bar = (hn_top_bar*)object;
    }
    else
    {
        hn_server_client_destroy(client);
        return;
    }

    object->active = HN_TRUE;

    client->server->requests_interface->client_object_set_active(client, object);
}

void hn_object_set_enabled_handler(hn_client *client)
{
    hn_object_id object_id;

    if(hn_client_read(client, &object_id, sizeof(hn_object_id)))
        return;

    struct hn_object_struct *object = hn_object_get_by_id(client, object_id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type != HN_OBJECT_TYPE_MENU &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_bool enabled;

    if(hn_client_read(client, &enabled, sizeof(hn_bool)))
        return;

    if(enabled != HN_TRUE && enabled != HN_FALSE)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(enabled == object->enabled)
    {
        hn_server_client_destroy(client);
        return;
    }

    if(object->type == HN_OBJECT_TYPE_OPTION)
    {
        if(enabled)
        {
            hn_select *sel = (hn_select*)object->parent;

            if(sel && !sel->active_option)
            {
                sel->active_option = (hn_option*)object;
                object->active = HN_TRUE;
                client->server->requests_interface->client_object_set_active(client, object);
            }
        }
        else
        {
            hn_select *sel = (hn_select*)object->parent;

            if(sel && sel->active_option == (hn_option*)object)
            {
                hn_node *node = sel->object.children->end;
                hn_option *option;

                while(node)
                {
                    option = node->data;

                    if(option->object.enabled)
                    {
                        sel->active_option = option;
                        option->object.active = HN_TRUE;
                        object->active = HN_FALSE;
                        client->server->requests_interface->client_object_set_active(client, option);
                        goto skip;
                    }

                    node = node->prev;
                }

                sel->active_option = NULL;
                object->active = HN_FALSE;
            }
        }
    }

    skip:
    object->enabled = enabled;

    client->server->requests_interface->client_object_set_enabled(client, object, enabled);
}

void hn_object_destroy(hn_object *obj)
{
    struct hn_object_struct *object = (struct hn_object_struct *)obj;

    switch(object->type)
    {
        case HN_OBJECT_TYPE_TOP_BAR:
        {
            hn_top_bar_before_destroy((hn_top_bar*) object);
        }break;
        case HN_OBJECT_TYPE_MENU:
        {
            hn_menu_before_destroy((hn_menu*) object);
        }break;
        case HN_OBJECT_TYPE_ACTION:
        {
            hn_action_before_destroy((hn_action*)object);
        }break;
        case HN_OBJECT_TYPE_TOGGLE:
        {
            hn_toggle_before_destroy((hn_toggle*)object);
        }break;
        case HN_OBJECT_TYPE_SELECT:
        {
            hn_select_before_destroy((hn_select*)object);
        }break;
        case HN_OBJECT_TYPE_OPTION:
        {
            hn_option_before_destroy((hn_option*)object);
        }break;
        case HN_OBJECT_TYPE_SEPARATOR:
        {
            hn_separator_before_destroy((hn_separator*)object);
        }break;
    }

    hn_array_erase(object->client->objects, object->link);

    hn_object_remove_from_parent(object);

    while(!hn_array_empty(object->children))
    {
        struct hn_object_struct *child = object->children->end->data;
        hn_object_remove_from_parent(child);
    }

    object->client->server->requests_interface->client_object_destroy(object->client, object);

    hn_array_destroy(object->children);

    free(object);
}

void hn_object_destroy_handler(hn_client *client)
{
    hn_object_id id;

    if(hn_client_read(client, &id, sizeof(hn_object_id)))
        return;

    hn_object *object = hn_object_get_by_id(client, id);

    if(!object)
    {
        hn_server_client_destroy(client);
        return;
    }

    hn_object_destroy(object);
}

/* COMPOSITOR REQUESTS */

void hn_set_active_client_handler(hn_compositor *compositor)
{
    hn_client_pid client_pid;

    if(hn_compositor_read(compositor, &client_pid, sizeof(hn_client_pid)))
        return;

    if(compositor->server->active_client)
    {
        if(client_pid == compositor->server->active_client->creds.pid)
            return;
    }

    if(client_pid == 0)
    {
        compositor->server->active_client = NULL;
        compositor->server->requests_interface->compositor_set_active_client(compositor, NULL, 0);
        return;
    }

    compositor->server->active_client = hn_client_get_by_pid(compositor->server,client_pid);
    compositor->server->requests_interface->compositor_set_active_client(compositor, compositor->server->active_client, client_pid);
}

void hn_compositor_send_custom_request_handler(hn_compositor *compositor)
{
    hn_string_length data_len;

    if(hn_compositor_read(compositor, &data_len, sizeof(u_int32_t)))
        return;

    if(data_len == 0)
        return;

    u_int8_t data[data_len];

    if(hn_compositor_read(compositor, data, data_len))
        return;

    compositor->server->requests_interface->compositor_send_custom_request(compositor, data, data_len);
}

/* CONNECTION */

void hn_client_handle_request(hn_client *client, hn_message_id msg_id)
{
    switch(msg_id)
    {
        case HN_CLIENT_REQUEST_SET_APP_NAME_ID:
        {
            hn_client_set_app_name_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_SEND_CUSTOM_REQUEST_ID:
        {
            hn_client_send_custom_request_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_CREATE_ID:
        {
            hn_object_create_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_PARENT_ID:
        {
            hn_object_set_parent_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_LABEL_ID:
        {
            hn_object_set_label_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_ICON_ID:
        {
            hn_object_set_icon_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_SHORTCUTS_ID:
        {
            hn_object_set_shortcuts_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_ENABLED_ID:
        {
            hn_object_set_enabled_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_CHECKED_ID:
        {
            hn_object_set_checked_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_SET_ACTIVE_ID:
        {
            hn_object_set_active_handler(client);
            return;
        }break;
        case HN_CLIENT_REQUEST_OBJECT_DESTROY_ID:
        {
            hn_object_destroy_handler(client);
            return;
        }break;
    }

    hn_server_client_destroy(client);
}

void hn_compositor_handle_request(hn_compositor *compositor, hn_message_id msg_id)
{
    switch(msg_id)
    {
        case HN_SET_ACTIVE_CLIENT_ID:
        {
            hn_set_active_client_handler(compositor);
            return;
        }break;
        case HN_COMPOSITOR_SEND_CUSTOM_REQUEST_ID:
        {
            hn_compositor_send_custom_request_handler(compositor);
            return;
        }break;
    }

    hn_server_compositor_destroy(compositor);
}

int hn_server_accept_connection(hn_server *server)
{
    int connection_fd = accept(server->fds[0].fd, NULL, NULL);

    if(connection_fd == -1)
        return -1;

    hn_object_type type;

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

    if(read(connection_fd, &type, sizeof(hn_object_type)) != sizeof(hn_object_type))
    {
        close(connection_fd);
        return -2;
    }

    /* CREATE CLIENT */

    if(type == HN_CONNECTION_TYPE_CLIENT)
    {
        int fds_i = hn_fds_add_fd(server->fds, connection_fd);

        if(fds_i == -1)
        {
            close(connection_fd);
            printf("Error: Clients limit reached.\n");
            return -2;
        }

        struct epoll_event epoll_ev;
        epoll_ev.events = POLLIN | POLLHUP;
        epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, connection_fd, &epoll_ev);

        hn_client *client = malloc(sizeof(hn_client));
        client->fds_i = fds_i;
        server->clients[fds_i - 2] = client;
        client->server = server;
        client->top_bars = hn_array_create();
        client->objects = hn_array_create();
        client->active_top_bar = NULL;

        socklen_t len = sizeof(client->creds);

        getsockopt(client->server->fds[client->fds_i].fd, SOL_SOCKET, SO_PEERCRED, &client->creds, &len);

        server->requests_interface->client_connected(client);

        return 0;
    }
    else if(type == HN_CONNECTION_TYPE_COMPOSITOR)
    {
        hn_compositor_auth_reply reply;

        // There is already a current compositor connected
        if(server->compositor)
        {
            reply = HN_COMPOSITOR_REJECTED;
            write(connection_fd, &reply, sizeof(hn_compositor_auth_reply));
            close(connection_fd);
            return -1;
        }


        struct epoll_event epoll_ev;
        epoll_ev.events = POLLIN | POLLHUP;
        epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, connection_fd, &epoll_ev);

        hn_compositor *compositor = malloc(sizeof(hn_compositor));
        compositor->server = server;

        server->compositor = compositor;
        server->fds[1].fd = connection_fd;
        server->fds[1].events = POLLIN | POLLHUP;
        server->fds[1].revents = 0;

        reply = HN_COMPOSITOR_ACCEPTED;
        write(connection_fd, &reply, sizeof(hn_compositor_auth_reply));

        server->requests_interface->compositor_connected(compositor);

        return 0;
    }
    else
    {
        close(connection_fd);
        return -2;
    }

}

int hn_server_dispatch_requests(hn_server *server, int timeout)
{
    int ret = poll(server->fds, HN_MAX_CLIENTS+2, timeout);

    if(ret == -1)
        return -1;

    if(ret != 0)
    {
        // New client or compositor
        if(server->fds[0].revents & POLLIN)
            hn_server_accept_connection(server);

        // If compositor
        if(server->compositor)
        {
            if(server->fds[1].revents & POLLHUP)
            {
                hn_server_compositor_destroy(server->compositor);
                goto read_clients;
            }

            if(server->fds[1].revents & POLLIN)
            {
                hn_message_id msg_id;

                if(hn_compositor_read(server->compositor, &msg_id, sizeof(hn_message_id)) == 0)
                    hn_compositor_handle_request(server->compositor, msg_id);

            }
        }

        read_clients:

        for(int i = 2; i < HN_MAX_CLIENTS+2; i++)
        {
            if(server->fds[i].fd != -1)
            {
                hn_client *client = server->clients[i-2];

                // Client disconnected
                if(server->fds[i].revents & POLLHUP)
                {
                    hn_server_client_destroy(client);
                    continue;
                }

                // Client request
                if(server->fds[i].revents & POLLIN)
                {
                    hn_message_id msg_id;

                    if(hn_client_read(client, &msg_id, sizeof(hn_message_id)))
                        continue;

                    hn_client_handle_request(client, msg_id);
                }
            }
        }
    }

    return 0;
}

void hn_server_compositor_destroy(hn_compositor *compositor)
{
    epoll_ctl(compositor->server->epoll_fd, EPOLL_CTL_DEL, compositor->server->fds[1].fd, NULL);
    close(compositor->server->fds[1].fd);
    compositor->server->fds[1].fd = -1;
    compositor->server->compositor = NULL;
    compositor->server->requests_interface->compositor_disconnected(compositor);
    free(compositor);
}

void hn_server_client_destroy(hn_client *client)
{
    if(client->server->active_client == client)
    {
        client->server->active_client = NULL;
        client->server->requests_interface->compositor_set_active_client(client->server->compositor, NULL, 0);
    }

    epoll_ctl(client->server->epoll_fd, EPOLL_CTL_DEL, client->server->fds[client->fds_i].fd, NULL);
    close(client->server->fds[client->fds_i].fd);
    client->server->fds[client->fds_i].fd = -1;

    while(!hn_array_empty(client->objects))
        hn_object_destroy(client->objects->end->data);

    hn_array_destroy(client->top_bars);

    hn_array_destroy(client->objects);

    client->server->requests_interface->client_disconnected(client);

    free(client);
}

/* EVENTS */

int hn_action_invoke(hn_action *action)
{
    if(!action)
        return HN_ERROR;

    if(hn_object_get_type(action) != HN_OBJECT_TYPE_ACTION)
        return HN_ERROR;

    if(action->object.enabled != HN_TRUE)
        return HN_ERROR;

    hn_message_id msg_id = HN_ACTION_INVOKE_ID;
    hn_server *server = action->object.client->server;

    pthread_mutex_lock(&server->mutex);

    if(hn_client_write(action->object.client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_client_write(action->object.client, &action->object.id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&server->mutex);

    return HN_SUCCESS;
}

int hn_server_send_custom_event_to_client(hn_client *client, void *data, u_int32_t size)
{
    if(!client)
        return HN_ERROR;

    if(size == 0)
        return HN_SUCCESS;

    if(!data)
        return HN_ERROR;

    // Memory access test
    u_int8_t *test = data;
    test = &test[size-1];
    HN_UNUSED(test);

    hn_message_id msg_id = HN_SERVER_TO_CLIENT_SEND_CUSTOM_EVENT_ID;

    hn_server *server = client->server;

    pthread_mutex_lock(&server->mutex);

    if(hn_client_write(client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_client_write(client, &size, sizeof(u_int32_t)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_client_write(client, data, size))
    {
        pthread_mutex_unlock(&server->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&server->mutex);

    return HN_SUCCESS;
}

int hn_server_send_custom_event_to_compositor(hn_compositor *compositor, void *data, u_int32_t size)
{
    if(!compositor)
        return HN_ERROR;

    if(size == 0)
        return HN_SUCCESS;

    if(!data)
        return HN_ERROR;

    // Memory access test
    u_int8_t *test = data;
    test = &test[size-1];
    HN_UNUSED(test);

    hn_message_id msg_id = HN_SERVER_TO_COMPOSITOR_SEND_CUSTOM_EVENT_ID;

    pthread_mutex_lock(&compositor->server->mutex);

    if(hn_compositor_write(compositor, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&compositor->server->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_compositor_write(compositor, &size, sizeof(u_int32_t)))
    {
        pthread_mutex_unlock(&compositor->server->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_compositor_write(compositor, data, size))
    {
        pthread_mutex_unlock(&compositor->server->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&compositor->server->mutex);

    return HN_SUCCESS;
}

