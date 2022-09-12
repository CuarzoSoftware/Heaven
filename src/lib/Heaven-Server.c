#include "Heaven-Server.h"
#include <asm-generic/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <pthread.h>

struct hv_server_struct
{
    int epoll_fd;
    struct pollfd fds[HV_MAX_CLIENTS+2];
    struct sockaddr_un name;
    hv_server_requests_interface *requests_interface;
    hv_client *clients[HV_MAX_CLIENTS];
    void *user_data;
    pthread_mutex_t mutex;
};

struct hv_creds
{
    pid_t pid;    /* process ID of the sending process */
    uid_t uid;    /* user ID of the sending process */
    gid_t gid;    /* group ID of the sending process */
};

struct hv_client_struct
{
    hv_array *objects;
    int fds_i;
    void *user_data;
    hv_server *server;
    hv_array *top_bars;
    hv_top_bar *active_top_bar;
    struct hv_creds creds;
};

struct hv_compositor_struct
{
    int a;
};

/* objects */

struct hv_object_struct
{
    hv_object_type type;
    hv_object_id id;
    hv_client *client;
    struct hv_object_struct *parent;
    hv_node *link;
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
    hv_action_state state;
};

struct hv_separator_struct
{
    struct hv_object_struct object;
};

/* UTILS */

int hv_fds_add_fd(struct pollfd *fds, int fd)
{
    for(int i = 2; i < HV_MAX_CLIENTS+2; i++)
    {
        if(fds[i].fd == -1)
        {
            fds[i].fd = fd;
            return i;
        }
    }

    return -1;
}

int hv_client_read(hv_client *client, void *dst, ssize_t bytes)
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
        hv_server_client_destroy(client);
        return 1;
    }

    u_int8_t *data = dst;

    ssize_t readBytes = read(client->server->fds[client->fds_i].fd, data, bytes);

    if(readBytes < 1)
    {
        printf("Error: Client read failed.\n");
        hv_server_client_destroy(client);
        return 1;
    }

    else if(readBytes < bytes)
        return hv_client_read(client, &data[readBytes], bytes - readBytes);

    return 0;
}

int hv_client_write(hv_client *client, void *src, ssize_t bytes)
{
    u_int8_t *data = src;

    ssize_t writtenBytes = write(client->server->fds[client->fds_i].fd, data, bytes);

    if(writtenBytes < 1)
    {
        hv_server_client_destroy(client);
        return 1;
    }
    else if(writtenBytes < bytes)
        return hv_client_write(client, &data[writtenBytes], bytes - writtenBytes);

    return 0;
}

/* SERVER */

hv_server *hv_server_create(const char *socket_name, void *user_data, hv_server_requests_interface *requests_interface)
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
        sock_name = HV_DEFAULT_SOCKET;

    char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    if(!xdg_runtime_dir)
    {
        printf("Error: XDG_RUNTIME_DIR env not set.\n");
        return NULL;
    }

    hv_server *server = malloc(sizeof(hv_server));
    server->requests_interface = requests_interface;
    server->fds[0].events = POLLIN;
    server->fds[0].revents = 0;
    server->user_data = user_data;

    if(pthread_mutex_init(&server->mutex, NULL) != 0)
    {
        printf("Error: Could not create mutex.\n");
        free(server);
        return NULL;
    }

    for(int i = 1; i < HV_MAX_CLIENTS+2; i++)
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

void *hv_server_get_user_data(hv_server *server)
{
    return server->user_data;
}

void hv_server_set_user_data(hv_server *server, void *user_data)
{
    server->user_data = user_data;
}

/* CLIENT */

void hv_client_get_credentials(hv_client *client, pid_t *pid, uid_t *uid, gid_t *gid)
{
    if(pid)
        *pid = client->creds.pid;
    if(uid)
        *uid = client->creds.uid;
    if(gid)
        *gid = client->creds.gid;
}

hv_client_id hv_client_get_id(hv_client *client)
{
    return client->fds_i-1;
}

hv_object *hv_client_object_create(hv_client *client, hv_object_type type, u_int32_t size)
{
    hv_object_id id;

    if(hv_client_read(client, &id, sizeof(hv_object_id)))
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

void hv_object_remove_from_parent(struct hv_object_struct *object)
{
    if(!object)
        return;

    if(!object->parent)
        return;

    object->client->server->requests_interface->client_object_remove_from_parent(object->client, object);

    hv_array_erase(object->parent->children, object->parent_link);

    object->parent_link = NULL;
    object->parent = NULL;
}

void hv_client_set_app_name_handler(hv_client *client)
{
    hv_string_length app_name_len;

    if(hv_client_read(client, &app_name_len, sizeof(hv_string_length)))
        return;

    char app_name[app_name_len+1];

    if(hv_client_read(client, app_name, app_name_len))
        return;

    app_name[app_name_len] = '\0';

    client->server->requests_interface->client_set_app_name(client, app_name);
}

void hv_client_send_custom_request_handler(hv_client *client)
{
    hv_string_length data_len;

    if(hv_client_read(client, &data_len, sizeof(u_int32_t)))
        return;

    if(data_len == 0)
        return;

    u_int8_t data[data_len];

    if(hv_client_read(client, data, data_len))
        return;

    client->server->requests_interface->client_send_custom_request(client, data, data_len);
}

/* TOP BAR */

void hv_top_bar_create_handler(hv_client *client)
{
    hv_top_bar *top_bar = (hv_top_bar *)hv_client_object_create(client, HV_OBJECT_TYPE_TOP_BAR, sizeof(hv_top_bar));
    if(!top_bar) return;
    top_bar->object.parent_link = hv_array_push_back(client->top_bars, top_bar);
    client->server->requests_interface->client_object_create(client, top_bar);
    if(client->active_top_bar == NULL)
    {
        client->active_top_bar = top_bar;
        client->server->requests_interface->client_top_bar_set_active(client, top_bar);
    }
}

void hv_top_bar_set_active_handler(hv_client *client)
{
    hv_object_id top_bar_id;

    if(hv_client_read(client, &top_bar_id, sizeof(hv_object_id)))
        return;

    hv_top_bar *top_bar = (hv_top_bar *)hv_object_get_by_id(client, top_bar_id);

    if(!top_bar)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(top_bar == client->active_top_bar)
        return;

    client->active_top_bar = top_bar;
    client->server->requests_interface->client_top_bar_set_active(client, top_bar);

}

void hv_top_bar_before_destroy(hv_top_bar *top_bar)
{
    hv_client *client = hv_object_get_client(top_bar);
    hv_array_erase(top_bar->object.client->top_bars, top_bar->object.parent_link);

    if(top_bar->object.client->active_top_bar == top_bar)
    {
        if(!hv_array_empty(top_bar->object.client->top_bars))
        {
            top_bar->object.client->active_top_bar = top_bar->object.client->top_bars->end->data;
            top_bar->object.client->server->requests_interface->client_top_bar_set_active(client, top_bar->object.client->active_top_bar);
        }
        else
        {
            top_bar->object.client->active_top_bar = NULL;
            top_bar->object.client->server->requests_interface->client_top_bar_set_active(client, NULL);
        }
    }
}

/* MENU */

void hv_menu_create_handler(hv_client *client)
{
    hv_menu *object = (hv_menu *)hv_client_object_create(client, HV_OBJECT_TYPE_MENU, sizeof(hv_menu));
    if(!object) return;
    client->server->requests_interface->client_object_create(client, object);
}

void hv_menu_set_title_handler(hv_client *client)
{
    hv_object_id id;

    if(hv_client_read(client, &id, sizeof(hv_object_id)))
        return;

    hv_menu *menu = (hv_menu *)hv_object_get_by_id(client, id);

    if(!menu)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_string_length title_len;

    if(hv_client_read(client, &title_len, sizeof(hv_string_length)))
        return;

    if(title_len == 0)
        client->server->requests_interface->client_menu_set_title(client, menu, NULL);
    else
    {
        char title[title_len+1];

        if(hv_client_read(client, title, title_len))
            return;

        title[title_len] = '\0';

        client->server->requests_interface->client_menu_set_title(client, menu, title);
    }
}

void hv_menu_add_to_top_bar_handler(hv_client *client)
{
    hv_object_id menu_id;

    if(hv_client_read(client, &menu_id, sizeof(hv_object_id)))
        return;

    hv_menu *menu = (hv_menu *)hv_object_get_by_id(client, menu_id);

    if(!menu)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_id top_bar_id;

    if(hv_client_read(client, &top_bar_id, sizeof(hv_object_id)))
        return;

    hv_top_bar *top_bar = (hv_top_bar *)hv_object_get_by_id(client, top_bar_id);

    if(!top_bar)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_id before_id;

    if(hv_client_read(client, &before_id, sizeof(hv_object_id)))
        return;

    hv_menu *before = NULL;

    if(before_id == 0)
    {
        // Remove from current parent
        hv_object_remove_from_parent(&menu->object);

        // Set parent
        menu->object.parent = (struct hv_object_struct *)top_bar;

        // Push back
        menu->object.parent_link = hv_array_push_back(menu->object.parent->children, menu);

    }
    else
    {
        before = (hv_menu *)hv_object_get_by_id(client, before_id);

        if(!before)
        {
            hv_server_client_destroy(client);
            return;
        }

        if(before->object.type != HV_OBJECT_TYPE_MENU)
        {
            hv_server_client_destroy(client);
            return;
        }

        if(before->object.parent != (struct hv_object_struct *)top_bar)
        {
            hv_server_client_destroy(client);
            return;
        }

        // Remove from current parent
        hv_object_remove_from_parent(&menu->object);

        // Set parent
        menu->object.parent = (struct hv_object_struct *)top_bar;

        // Insert before before
        menu->object.parent_link = hv_array_insert_before(menu->object.parent->children, before->object.parent_link, menu);

    }

    client->server->requests_interface->client_menu_add_to_top_bar(client, menu, top_bar, before);
}

void hv_menu_add_to_action_handler(hv_client *client)
{
    hv_object_id menu_id;

    if(hv_client_read(client, &menu_id, sizeof(hv_object_id)))
        return;

    hv_menu *menu = (hv_menu *)hv_object_get_by_id(client, menu_id);

    if(!menu)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_id action_id;

    if(hv_client_read(client, &action_id, sizeof(hv_object_id)))
        return;

    hv_action *action = (hv_action *)hv_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_remove_from_parent(&menu->object);

    if(!hv_array_empty(action->object.children))
        hv_object_remove_from_parent(action->object.children->end->data);

    menu->object.parent = (struct hv_object_struct*)action;
    menu->object.parent_link = hv_array_push_back(action->object.children, menu);

    client->server->requests_interface->client_menu_add_to_action(client, menu, action);
}

void hv_menu_before_destroy(hv_menu *menu)
{
    HV_UNUSED(menu);
}

/* ACTION */

void hv_action_create_handler(hv_client *client)
{
    hv_action *action = (hv_action *)hv_client_object_create(client, HV_OBJECT_TYPE_ACTION, sizeof(hv_action));
    action->state = HV_ACTION_STATE_ENABLED;
    if(!action) return;
    client->server->requests_interface->client_object_create(client, action);
}

void hv_action_set_text_handler(hv_client *client)
{
    hv_object_id action_id;

    if(hv_client_read(client, &action_id, sizeof(hv_object_id)))
        return;

    hv_action *action = (hv_action*)hv_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_string_length text_len;

    if(hv_client_read(client, &text_len, sizeof(hv_string_length)))
        return;

    if(text_len == 0)
    {
        client->server->requests_interface->client_action_set_text(client, action, NULL);
        return;
    }

    char text[text_len + 1];

    if(hv_client_read(client, &text, text_len))
        return;

    text[text_len] = '\0';

    client->server->requests_interface->client_action_set_text(client, action, text);
}

void hv_action_set_icon_handler(hv_client *client)
{
    hv_object_id action_id;

    if(hv_client_read(client, &action_id, sizeof(hv_object_id)))
        return;

    hv_action *action = (hv_action*)hv_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_server_client_destroy(client);
        return;
    }

    u_int32_t width;

    if(hv_client_read(client, &width, sizeof(u_int32_t)))
        return;

    if(width == 0)
    {
        client->server->requests_interface->client_action_set_icon(client, action, NULL, 0, 0);
        return;
    }

    u_int32_t height;

    if(hv_client_read(client, &height, sizeof(u_int32_t)))
        return;

    if(height == 0)
    {
        hv_server_client_destroy(client);
        return;
    }

    u_int32_t total_pixels = width*height;

    hv_pixel pixels[total_pixels];

    if(hv_client_read(client, pixels, total_pixels))
        return;

    client->server->requests_interface->client_action_set_icon(client, action, pixels, width, height);
}

void hv_action_set_shortcuts_handler(hv_client *client)
{
    hv_object_id action_id;

    if(hv_client_read(client, &action_id, sizeof(hv_object_id)))
        return;

    hv_action *action = (hv_action*)hv_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_string_length shortcuts_len;

    if(hv_client_read(client, &shortcuts_len, sizeof(hv_string_length)))
        return;

    if(shortcuts_len == 0)
    {
        client->server->requests_interface->client_action_set_shortcuts(client, action, NULL);
        return;
    }

    char shortcuts[shortcuts_len+1];

    if(hv_client_read(client, shortcuts, shortcuts_len))
        return;

    shortcuts[shortcuts_len] = '\0';

    client->server->requests_interface->client_action_set_shortcuts(client, action, shortcuts);
}

void hv_action_set_state_handler(hv_client *client)
{
    hv_object_id action_id;

    if(hv_client_read(client, &action_id, sizeof(hv_object_id)))
        return;

    hv_action *action = (hv_action*)hv_object_get_by_id(client, action_id);

    if(!action)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_action_state state;

    if(hv_client_read(client, &state, sizeof(hv_action_state)))
        return;

    if(state != HV_ACTION_STATE_ENABLED && state != HV_ACTION_STATE_DISABLED)
        return;

    if(state == action->state)
        return;

    client->server->requests_interface->client_action_set_state(client, action, state);
}

void hv_action_before_destroy(hv_action *action)
{
    HV_UNUSED(action);
}

/* SEPARATOR */

void hv_separator_create_handler(hv_client *client)
{
    hv_separator *separator = (hv_separator *)hv_client_object_create(client, HV_OBJECT_TYPE_SEPARATOR, sizeof(hv_separator));
    if(!separator) return;
    client->server->requests_interface->client_object_create(client, separator);
}

void hv_separator_set_text_handler(hv_client *client)
{
    hv_object_id separator_id;

    if(hv_client_read(client, &separator_id, sizeof(hv_object_id)))
        return;

    hv_separator *separator = (hv_separator*)hv_object_get_by_id(client, separator_id);

    if(!separator)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_string_length text_len;

    if(hv_client_read(client, &text_len, sizeof(hv_string_length)))
        return;

    if(text_len == 0)
    {
        client->server->requests_interface->client_separator_set_text(client, separator, NULL);
        return;
    }

    char text[text_len + 1];

    if(hv_client_read(client, &text, text_len))
        return;

    text[text_len] = '\0';

    client->server->requests_interface->client_separator_set_text(client, separator, text);
}

void hv_separator_before_destroy(hv_separator *separator)
{
    HV_UNUSED(separator);
}

/* ITEM */

void hv_item_add_to_menu_handler(hv_client *client)
{
    hv_object_id item_id;

    if(hv_client_read(client, &item_id, sizeof(hv_object_id)))
        return;

    struct hv_object_struct *item = (struct hv_object_struct *)hv_object_get_by_id(client, item_id);

    if(!item)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(item->type != HV_OBJECT_TYPE_ACTION && item->type != HV_OBJECT_TYPE_SEPARATOR)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_id menu_id;

    if(hv_client_read(client, &menu_id, sizeof(hv_object_id)))
        return;

    hv_menu *menu = (hv_menu *)hv_object_get_by_id(client, menu_id);

    if(!menu)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(menu->object.type != HV_OBJECT_TYPE_MENU)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(menu->object.parent == item)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_id before_id;

    if(hv_client_read(client, &before_id, sizeof(hv_object_id)))
        return;

    if(before_id == 0)
    {
        hv_object_remove_from_parent(item);
        item->parent = (hv_object*)menu;
        item->parent_link = hv_array_push_back(menu->object.children, item);
        item->client->server->requests_interface->client_item_add_to_menu(client, item, menu, NULL);
        return;
    }

    struct hv_object_struct *before = (struct hv_object_struct *)hv_object_get_by_id(client, before_id);

    if(!before)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(before->type != HV_OBJECT_TYPE_ACTION && before->type != HV_OBJECT_TYPE_SEPARATOR)
    {
        hv_server_client_destroy(client);
        return;
    }

    if(before->parent != (hv_object*)menu)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_remove_from_parent(item);
    item->parent = (hv_object*)menu;
    item->parent_link = hv_array_insert_before(menu->object.children, before->parent_link ,item);
    item->client->server->requests_interface->client_item_add_to_menu(client, item, menu, NULL);
}

/* OBJECT */

hv_object *hv_object_get_user_data(hv_object *object)
{
    struct hv_object_struct *obj = object;
    return obj->user_data;
}

void hv_object_set_user_data(hv_object *object, void *user_data)
{
    struct hv_object_struct *obj = object;
    obj->user_data = user_data;
}

void hv_object_create_handler(hv_client *client)
{
    hv_object_type type;

    if(hv_client_read(client, &type, sizeof(hv_object_type)))
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
            hv_separator_create_handler(client);
            return;
        }break;
    }

    hv_server_client_destroy(client);
}

void hv_object_remove_from_parent_handler(hv_client *client)
{
    hv_object_id id;

    if(hv_client_read(client, &id, sizeof(hv_object_id)))
        return;

    struct hv_object_struct *object = hv_object_get_by_id(client, id);

    hv_object_remove_from_parent(object);
}

void hv_object_destroy(hv_object *obj)
{
    struct hv_object_struct *object = (struct hv_object_struct *)obj;

    switch(object->type)
    {
        case HV_OBJECT_TYPE_TOP_BAR:
        {
            hv_top_bar_before_destroy((hv_top_bar*) object);
        }break;
        case HV_OBJECT_TYPE_MENU:
        {
            hv_menu_before_destroy((hv_menu*) object);
        }break;
        case HV_OBJECT_TYPE_ACTION:
        {
            hv_action_before_destroy((hv_action*)object);
        }break;
        case HV_OBJECT_TYPE_SEPARATOR:
        {
            hv_separator_before_destroy((hv_separator*)object);
        }break;
    }

    hv_array_erase(object->client->objects, object->link);

    hv_object_remove_from_parent(object);

    while(!hv_array_empty(object->children))
    {
        struct hv_object_struct *child = object->children->end->data;
        hv_object_remove_from_parent(child);
    }

    object->client->server->requests_interface->client_object_destroy(object->client, object);

    hv_array_destroy(object->children);

    free(object);
}

void hv_object_destroy_handler(hv_client *client)
{
    hv_object_id id;

    if(hv_client_read(client, &id, sizeof(hv_object_id)))
        return;

    hv_object *object = hv_object_get_by_id(client, id);

    if(!object)
    {
        hv_server_client_destroy(client);
        return;
    }

    hv_object_destroy(object);
}

/* CONNECTION */

void hv_client_handle_request(hv_client *client, hv_message_id msg_id)
{
    switch(msg_id)
    {
        case HV_CLIENT_SET_APP_NAME_ID:
        {
            hv_client_set_app_name_handler(client);
        }break;
        case HV_CLIENT_SEND_CUSTOM_REQUEST_ID:
        {
            hv_client_send_custom_request_handler(client);
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
        case HV_OBJECT_REMOVE_FROM_PARENT_ID:
        {
            hv_object_remove_from_parent_handler(client);
        }break;
        case HV_MENU_SET_TITLE_ID:
        {
            hv_menu_set_title_handler(client);
        }break;
        case HV_MENU_ADD_TO_TOP_BAR_ID:
        {
            hv_menu_add_to_top_bar_handler(client);
        }break;
        case HV_MENU_ADD_TO_ACTION_ID:
        {
            hv_menu_add_to_action_handler(client);
        }break;
        case HV_ACTION_SET_TEXT_ID:
        {
            hv_action_set_text_handler(client);
        }break;
        case HV_ACTION_SET_ICON_ID:
        {
            hv_action_set_icon_handler(client);
        }break;
        case HV_ACTION_SET_SHORTCUTS_ID:
        {
            hv_action_set_shortcuts_handler(client);
        }break;
        case HV_ACTION_SET_STATE_ID:
        {
            hv_action_set_state_handler(client);
        }break;
        case HV_SEPARATOR_SET_TEXT_ID:
        {
            hv_separator_set_text_handler(client);
        }break;
        case HV_ITEM_ADD_TO_MENU_ID:
        {
            hv_item_add_to_menu_handler(client);
        }break;
    }
}

int hv_server_accept_connection(hv_server *server)
{
    int connection_fd = accept(server->fds[0].fd, NULL, NULL);

    if(connection_fd == -1)
        return -1;

    hv_object_type type;

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

    if(read(connection_fd, &type, sizeof(hv_object_type)) != sizeof(hv_object_type))
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

        socklen_t len = sizeof(client->creds);

        getsockopt(client->server->fds[client->fds_i].fd, SOL_SOCKET, SO_PEERCRED, &client->creds, &len);

        server->requests_interface->client_connected(client);

        return 0;
    }
    else
    {
        close(connection_fd);
        return -2;
    }

}

int hv_server_dispatch_requests(hv_server *server, int timeout)
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
                    hv_server_client_destroy(client);
                    continue;
                }

                // Client request
                if(server->fds[i].revents & POLLIN)
                {
                    hv_message_id msg_id;

                    if(hv_client_read(client, &msg_id, sizeof(hv_message_id)))
                        continue;

                    hv_client_handle_request(client, msg_id);
                }
            }
        }
    }

    return 0;
}

void hv_server_client_destroy(hv_client *client)
{
    if(client->active_top_bar)
    {
        client->active_top_bar = NULL;
        client->server->requests_interface->client_top_bar_set_active(client, NULL);
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


int hv_action_invoke(hv_action *action)
{
    if(!action)
        return HV_ERROR;

    if(hv_object_get_type(action) != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    if(action->state == HV_ACTION_STATE_DISABLED)
        return HV_ERROR;

    hv_message_id msg_id = HV_ACTION_INVOKE_ID;
    hv_server *server = action->object.client->server;

    pthread_mutex_lock(&server->mutex);

    if(hv_client_write(action->object.client, &msg_id, sizeof(hv_message_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_client_write(action->object.client, &action->object.id, sizeof(hv_object_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HV_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&server->mutex);

    return HV_SUCCESS;
}

int hv_server_send_custom_event(hv_client *client, void *data, u_int32_t size)
{
    if(!client)
        return HV_ERROR;

    if(size == 0)
        return HV_SUCCESS;

    if(!data)
        return HV_ERROR;

    // Memory access test
    u_int8_t *test = data;
    test = &test[size-1];
    HV_UNUSED(test);

    hv_message_id msg_id = HV_SERVER_TO_CLIENT_SEND_CUSTOM_EVENT_ID;

    hv_server *server = client->server;

    pthread_mutex_lock(&server->mutex);

    if(hv_client_write(client, &msg_id, sizeof(hv_message_id)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_client_write(client, &size, sizeof(u_int32_t)))
    {
        pthread_mutex_unlock(&server->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_client_write(client, data, size))
    {
        pthread_mutex_unlock(&server->mutex);
        return HV_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&server->mutex);

    return HV_SUCCESS;
}
