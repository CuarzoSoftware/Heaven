#include "Heaven-Client.h"
#include <poll.h>
#include <sys/select.h>
#include <pthread.h>

struct hn_client_struct
{
    hn_array *objects;
    void *user_data;
    struct pollfd fd;
    struct sockaddr_un name;
    hn_client_events_interface *events_interface;
    hn_array *free_ids;
    hn_object_id greatest_id;
    pthread_mutex_t mutex;
    hn_top_bar *active_top_bar;
    int destroyed;
    int initialized;
    char *error;
};

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
    void (*destroy_func)(hn_object *);
    hn_bool enabled;
    hn_bool active;
    hn_bool checked;
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

struct hn_toggle_struct
{
    struct hn_object_struct object;
};

struct hn_option_struct
{
    struct hn_object_struct object;
};

struct hn_select_struct
{
    struct hn_object_struct object;
    hn_option *active_option;
};

struct hn_separator_struct
{
    struct hn_object_struct object;
};

/* Utils */

int hn_server_read(hn_client *client, void *dst, ssize_t bytes)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(client->fd.fd, &set);

    if(select(client->fd.fd + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        printf("Error: Server read timeout.\n");
        hn_client_destroy(client);
        return 1;
    }

    u_int8_t *data = dst;
    ssize_t readBytes;

    readBytes = read(client->fd.fd , data, bytes);

    if(readBytes < 1)
    {
        printf("Error: Server read failed. read() call returned %zd\n", readBytes);
        hn_client_destroy(client);
        return 1;
    }
    else if(readBytes < bytes)
        hn_server_read(client, &data[readBytes], bytes - readBytes);

    return 0;
}

int hn_server_write(hn_client *client, void *src, ssize_t bytes)
{
    u_int8_t *data = src;

    ssize_t writtenBytes = write(client->fd.fd, data, bytes);

    if(writtenBytes < 1)
    {
        hn_client_destroy(client);
        return 1;
    }
    else if(writtenBytes < bytes)
        return hn_server_write(client, &data[writtenBytes], bytes - writtenBytes);

    return 0;
}

/* CLIENT */

hn_client *hn_client_create(const char *socket_name, const char *app_name, void *user_data, hn_client_events_interface *events_interface)
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

    hn_client *client = malloc(sizeof(hn_client));
    client->events_interface = events_interface;
    client->fd.events = POLLIN | POLLHUP;
    client->fd.revents = 0;
    client->active_top_bar = NULL;
    client->user_data = user_data;
    client->destroyed = 0;
    client->initialized = 0;
    client->error = "Unknown error";

    if(pthread_mutex_init(&client->mutex, NULL) != 0)
    {
        printf("Error: Could not create mutex.\n");
        free(client);
        return NULL;
    }

    if( (client->fd.fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(client);
        return NULL;
    }

    int xdg_runtime_dir_len = strlen(xdg_runtime_dir);

    memset(&client->name.sun_path, 0, 108);
    client->name.sun_family = AF_UNIX;
    memcpy(client->name.sun_path, xdg_runtime_dir, xdg_runtime_dir_len);

    if(xdg_runtime_dir[xdg_runtime_dir_len-1] != '/')
    {
        client->name.sun_path[xdg_runtime_dir_len] = '/';
        xdg_runtime_dir_len++;
    }

    strncpy(&client->name.sun_path[xdg_runtime_dir_len], sock_name, 107 - xdg_runtime_dir_len);

    if(connect(client->fd.fd, (const struct sockaddr*)&client->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: No server currently running in %s.\n", client->name.sun_path);
        free(client);
        return NULL;
    }

    /* IDENTIFY */

    client->objects = hn_array_create();
    client->free_ids = hn_array_create();
    client->greatest_id = 0;

    hn_connection_type type = HN_CONNECTION_TYPE_CLIENT;

    /* CONNECTION TYPE: UInt32 */

    if(hn_server_write(client, &type, sizeof(hn_connection_type)))
    {
        return NULL;
    }

    /* SEND APP NAME */
    int ret = hn_client_set_app_name(client, app_name);

    pthread_mutex_lock(&client->mutex);

    if(ret == HN_ERROR)
    {
        hn_client_destroy(client);
        printf("Error: App name is NULL.\n");
        pthread_mutex_unlock(&client->mutex);
        return NULL;
    }
    else if(ret == HN_CONNECTION_LOST)
    {
        pthread_mutex_unlock(&client->mutex);
        return NULL;
    }

    client->initialized = 1;

    pthread_mutex_unlock(&client->mutex);

    return client;
}

void hn_client_destroy(hn_client *client)
{
    client->destroyed = 1;

    // Destroy objects
    while(!hn_array_empty(client->objects))
        hn_object_destroy(client->objects->end->data);

    hn_array_destroy(client->objects);

    // Free freed IDs
    while(!hn_array_empty(client->free_ids))
    {
        free(client->free_ids->end->data);
        hn_array_pop_back(client->free_ids);
    }

    hn_array_destroy(client->free_ids);

    pthread_mutex_destroy(&client->mutex);

    close(client->fd.fd);

    if(client->initialized)
        client->events_interface->disconnected_from_server(client);

    free(client);
}

int hn_client_set_app_name(hn_client *client, const char *app_name)
{
    if(app_name == NULL)
        return HN_ERROR;

    pthread_mutex_lock(&client->mutex);

    hn_message_id msg_id = HN_CLIENT_REQUEST_SET_APP_NAME_ID;
    hn_string_length app_name_len;
    u_int32_t app_name_len_32 = strlen(app_name);

    if(app_name_len_32 > 255)
        app_name_len = 255;
    else
        app_name_len = app_name_len_32;

    /* MSG ID: UInt32
     * APP NAME LENGHT: UInt32
     * APP NAME: APP NAME LENGHT */

    if(hn_server_write(client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(client, &app_name_len, sizeof(hn_string_length)))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(client, (char*)app_name, app_name_len))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&client->mutex);

    return HN_SUCCESS;
}

int hn_client_send_custom_request(hn_client *client, void *data, u_int32_t size)
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

    hn_message_id msg_id = HN_CLIENT_REQUEST_SEND_CUSTOM_REQUEST_ID;

    pthread_mutex_lock(&client->mutex);

    if(hn_server_write(client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(client, &size, sizeof(u_int32_t)))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(client, data, size))
    {
        pthread_mutex_unlock(&client->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&client->mutex);

    return HN_SUCCESS;
}

int hn_client_get_fd(hn_client *client)
{
    return client->fd.fd;
}

/* UTILS */

hn_object_id hn_object_new_id(hn_client *client)
{
    hn_object_id id;

    if(hn_array_empty(client->free_ids))
    {
        client->greatest_id++;
        id = client->greatest_id;
    }
    else
    {
        id = *(hn_object_id*)client->free_ids->end->data;
        free(client->free_ids->end->data);
        hn_array_pop_back(client->free_ids);
    }

    return id;
}

void hn_object_free_id(hn_client *client, hn_object_id id)
{
    // Free ID
    if(id == client->greatest_id)
        client->greatest_id--;
    else
    {
        hn_object_id *freed_id = malloc(sizeof(hn_object_id));
        *freed_id = id;
        hn_array_push_back(client->free_ids, freed_id);
    }
}

hn_object *hn_client_object_create(hn_client *client, hn_object_type type, u_int32_t size, void *user_data, void (*destroy_func)(hn_object *))
{
    pthread_mutex_lock(&client->mutex);

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_CREATE_ID;

    struct hn_object_struct *object = malloc(size);
    object->type = type;
    object->id = hn_object_new_id(client);
    object->link = hn_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hn_array_create();
    object->user_data = user_data;
    object->destroy_func = destroy_func;
    object->enabled = HN_TRUE;
    object->active = HN_FALSE;
    object->checked = HN_FALSE;

    /* MSG ID: UInt32
     * OBJECT TYPE ID: UInt32
     * MENU BAR ID: UInt32 */

    if(hn_server_write(client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&client->mutex);
        return NULL;
    }

    if(hn_server_write(client, &type, sizeof(hn_object_type)))
    {
        pthread_mutex_unlock(&client->mutex);
        return NULL;
    }

    if(hn_server_write(client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&client->mutex);
        return NULL;
    }

    pthread_mutex_unlock(&client->mutex);

    return (hn_object*)object;
}

/* OBJECT */

int hn_object_set_parent_send(struct hn_object_struct *object, struct hn_object_struct *parent, struct hn_object_struct *before)
{
    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_PARENT_ID;

    pthread_mutex_lock(&object->client->mutex);

    // Remove from current parent if any
    if(object->parent)
        hn_array_erase(object->parent->children, object->parent_link);

    object->parent = parent;

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(parent)
    {
        if(hn_server_write(object->client, &parent->id, sizeof(hn_object_id)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }

        if(before)
        {
            // Insert before before
            object->parent_link = hn_array_insert_before(parent->children, before->parent_link, object);

            if(hn_server_write(object->client, &before->id, sizeof(hn_object_id)))
            {
                pthread_mutex_unlock(&object->client->mutex);
                return HN_CONNECTION_LOST;
            }
        }
        else
        {
            // Insert at the end
            object->parent_link = hn_array_push_back(parent->children, object);

            hn_object_id null_id = 0;
            if(hn_server_write(object->client, &null_id, sizeof(hn_object_id)))
            {
                pthread_mutex_unlock(&object->client->mutex);
                return HN_CONNECTION_LOST;
            }
        }
    }
    else
    {
        // Set NULL parent link
        object->parent_link = NULL;

        hn_object_id null_id = 0;
        if(hn_server_write(object->client, &null_id, sizeof(hn_object_id)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_parent(hn_object *obj, hn_object *par, hn_object *bef)
{
    struct hn_object_struct *object = obj;
    struct hn_object_struct *parent = par;
    struct hn_object_struct *before = bef;

    if(!object)
    {
        return HN_ERROR;
    }

    hn_client *client = object->client;

    if(object == parent || object == before)
    {
        client->error = "Object can not be child of itself";
        return HN_ERROR;
    }

    if(parent)
    {
        if(parent->client != object->client)
        {
            client->error = "Object and parent have different clients";
            return HN_ERROR;
        }

        if(before)
        {
            if(before->parent != parent)
            {
                client->error = "Before is not child of parent";
                return HN_ERROR;
            }

            if(object == before)
            {
                client->error = "Before can not be equal to object";
                return HN_ERROR;
            }

            if(object->parent_link && parent->parent_link->next && parent->parent_link->next->data == before)
                return HN_SUCCESS;
        }
        else
        {
            if(parent->children->end && parent->children->end->data == object)
                return HN_SUCCESS;
        }
    }
    else
    {
        if(object->parent == NULL)
            return HN_SUCCESS;
    }



    switch (object->type)
    {
        case HN_OBJECT_TYPE_MENU:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_TOP_BAR && parent->type != HN_OBJECT_TYPE_MENU)
            {
                client->error = "Menus can only be child of top bars or menus";
                return HN_ERROR;
            }

            return hn_object_set_parent_send(object, parent, before);
        }break;
        case HN_OBJECT_TYPE_ACTION:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_MENU)
                return HN_ERROR;

            return hn_object_set_parent_send(object, parent, before);
        }break;
        case HN_OBJECT_TYPE_TOGGLE:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_MENU)
                return HN_ERROR;

            return hn_object_set_parent_send(object, parent, before);
        }break;
        case HN_OBJECT_TYPE_SELECT:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_MENU)
                return HN_ERROR;

            return hn_object_set_parent_send(object, parent, before);
        }break;
        case HN_OBJECT_TYPE_OPTION:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_SELECT)
                return HN_ERROR;

            return hn_object_set_parent_send(object, parent, before);
        }break;
        case HN_OBJECT_TYPE_SEPARATOR:
        {
            if(parent && parent->type != HN_OBJECT_TYPE_MENU && parent->type != HN_OBJECT_TYPE_SELECT)
                return HN_ERROR;

            return hn_object_set_parent_send(object, parent, before);
        }break;
    };

    return HN_ERROR;
}

int hn_object_set_label(hn_object *obj, const char *label)
{
    struct hn_object_struct *object = obj;
    hn_string_length label_len = 0;

    if(!object)
        return HN_ERROR;

    hn_client *client = object->client;

    if(object->type != HN_OBJECT_TYPE_MENU &&
            object->type != HN_OBJECT_TYPE_SEPARATOR &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
    {
        client->error = "Label can not be assigned to this object";
        return HN_ERROR;
    }

    if(label)
    {
        u_int32_t label_len_32 = strlen(label);

        if(label_len_32 > 255)
            label_len = 255;
        else
            label_len = label_len_32;
    }

    pthread_mutex_lock(&object->client->mutex);

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_LABEL_ID;

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &label_len, sizeof(hn_string_length)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(label_len > 0)
    {
        if(hn_server_write(object->client, (char*)label, label_len))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_icon(hn_object *obj, hn_pixel *pixels, u_int32_t width, u_int32_t height)
{
    struct hn_object_struct *object = obj;

    if(!object)
        return HN_ERROR;

    if(object->type != HN_OBJECT_TYPE_MENU &&
            object->type != HN_OBJECT_TYPE_ACTION)
        return HN_ERROR;

    u_int32_t total_pixels = width*height;

    if(pixels)
    {
        if(total_pixels == 0)
            return HN_ERROR;

        // Memory access test
        hn_pixel test_pixel = pixels[total_pixels-1];
        (void)test_pixel;
    }

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_ICON_ID;

    pthread_mutex_lock(&object->client->mutex);

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(!pixels)
    {
        width = 0;

        if(hn_server_write(object->client, &width, sizeof(u_int32_t)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }
    else
    {
        if(hn_server_write(object->client, &width, sizeof(u_int32_t)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }

        if(hn_server_write(object->client, &height, sizeof(u_int32_t)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }

        if(hn_server_write(object->client, pixels, total_pixels))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_shortcuts(hn_object *obj, const char *shortcuts)
{
    struct hn_object_struct *object = obj;

    if(!object)
        return HN_ERROR;

    if(object->type != HN_OBJECT_TYPE_SEPARATOR &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
        return HN_ERROR;

    hn_string_length shortcuts_len = 0;

    if(shortcuts)
    {
        u_int32_t shortcuts_len_32 = 0;
        shortcuts_len_32 = strlen(shortcuts);

        if(shortcuts_len_32 > 255)
            shortcuts_len = 255;
        else
            shortcuts_len = shortcuts_len_32;
    }

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_SHORTCUTS_ID;

    pthread_mutex_lock(&object->client->mutex);

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &shortcuts_len, sizeof(hn_string_length)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(shortcuts)
    {
        if(hn_server_write(object->client, (char*)shortcuts, shortcuts_len))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_enabled(hn_object *obj, hn_bool enabled)
{
    struct hn_object_struct *object = obj;

    if(!object)
        return HN_ERROR;

    if(object->type != HN_OBJECT_TYPE_MENU &&
            object->type != HN_OBJECT_TYPE_ACTION &&
            object->type != HN_OBJECT_TYPE_TOGGLE &&
            object->type != HN_OBJECT_TYPE_OPTION)
        return HN_ERROR;

    if(object->enabled == enabled)
        return HN_SUCCESS;

    pthread_mutex_lock(&object->client->mutex);

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_ENABLED_ID;

    if(object->type == HN_OBJECT_TYPE_OPTION)
    {
        if(enabled)
        {
            hn_select *sel = (hn_select*)object->parent;

            if(sel && !sel->active_option)
            {
                sel->active_option = (hn_option*)object;
                object->active = HN_TRUE;
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

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->enabled, sizeof(hn_bool)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_active(hn_object *obj)
{
    printf("SET ACTIVE\n");
    struct hn_object_struct *object = obj;

    if(!object)
        return HN_ERROR;

    if(object->type == HN_OBJECT_TYPE_OPTION)
    {
        if(object->enabled != HN_TRUE)
            return HN_ERROR;

        if(!object->parent)
        {
            return HN_ERROR;
        }
        else
        {
            hn_select *sel = (hn_select*)object->parent;

            if(sel->active_option)
                sel->active_option->object.active = HN_FALSE;

            sel->active_option = (hn_option*)object;
        }
    }
    else if(object->type == HN_OBJECT_TYPE_TOP_BAR)
    {
        if(object->client->active_top_bar)
            object->client->active_top_bar->object.active = HN_FALSE;

        object->client->active_top_bar = (hn_top_bar*)object;
    }
    else
    {
        return HN_ERROR;
    }

    if(object->active == HN_TRUE)
        return HN_SUCCESS;

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_ACTIVE_ID;

    pthread_mutex_lock(&object->client->mutex);

    object->active = HN_TRUE;

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_set_checked(hn_object *obj, hn_bool checked)
{
    struct hn_object_struct *object = obj;

    if(!object)
        return HN_ERROR;

    if(object->type != HN_OBJECT_TYPE_TOGGLE)
        return HN_ERROR;

    if(object->checked == checked)
        return HN_SUCCESS;

    if(checked != HN_TRUE && checked != HN_FALSE)
        return HN_ERROR;

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_SET_CHECKED_ID;

    pthread_mutex_lock(&object->client->mutex);

    if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    if(hn_server_write(object->client, &checked, sizeof(hn_bool)))
    {
        pthread_mutex_unlock(&object->client->mutex);
        return HN_CONNECTION_LOST;
    }

    object->checked = checked;

    pthread_mutex_unlock(&object->client->mutex);

    return HN_SUCCESS;
}

int hn_object_remove_fom_parent(hn_object *object)
{
    return hn_object_set_parent(object, NULL, NULL);
}

int hn_object_destroy(hn_object *obj)
{
    struct hn_object_struct *object = (struct hn_object_struct*)obj;

    hn_message_id msg_id = HN_CLIENT_REQUEST_OBJECT_DESTROY_ID;

    if(object->client->destroyed)
        object->client->events_interface->object_destroy(object);

    pthread_mutex_lock(&object->client->mutex);

    if(object->parent)
        hn_array_erase(object->parent->children, object->parent_link);

    // Unlink children
    while(!hn_array_empty(object->children))
    {
        struct hn_object_struct *child = object->children->end->data;
        child->parent_link = NULL;
        child->parent = NULL;
        hn_array_pop_back(object->children);
    }

    hn_array_destroy(object->children);

    hn_object_free_id(object->client, object->id);

    hn_array_erase(object->client->objects, object->link);

    /* MSG ID: UInt32
     * OBJECT ID: UInt32 */

    if(!object->client->destroyed)
    {
        if(hn_server_write(object->client, &msg_id, sizeof(hn_message_id)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }

        if(hn_server_write(object->client, &object->id, sizeof(hn_object_id)))
        {
            pthread_mutex_unlock(&object->client->mutex);
            return HN_CONNECTION_LOST;
        }
    }

    object->destroy_func(object);

    pthread_mutex_unlock(&object->client->mutex);

    free(object);

    return HN_SUCCESS;
}

/* TOP BAR */

static void hn_top_bar_destroy_handler(hn_object *object)
{
    hn_top_bar *top_bar = (hn_top_bar*)object;

    if(top_bar == top_bar->object.client->active_top_bar)
    {
        hn_node *node = top_bar->object.client->objects->end;

        while(node)
        {
            struct hn_object_struct *obj = node->data;

            if(obj->type == HN_OBJECT_TYPE_TOP_BAR)
            {
                top_bar->object.client->active_top_bar = (hn_top_bar*)obj;
                return;
            }

            node = node->prev;
        }

        top_bar->object.client->active_top_bar = NULL;
    }

}

hn_top_bar *hn_top_bar_create(hn_client *client, void *user_data)
{
    hn_top_bar *top_bar = (hn_top_bar*) hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_TOP_BAR,
        sizeof(hn_top_bar),
        user_data,
        &hn_top_bar_destroy_handler
    );

    if(client->active_top_bar == NULL)
    {
        top_bar->object.active = HN_TRUE;
        client->active_top_bar = top_bar;
        //hn_top_bar_set_active(top_bar);
    }
    else
    {
        top_bar->object.active = HN_FALSE;
    }

    return top_bar;
}

int hn_top_bar_set_active(hn_top_bar *top_bar)
{
    if(!top_bar)
        return HN_ERROR;

    if(top_bar->object.client->active_top_bar == top_bar)
        return HN_ERROR;

    return hn_object_set_active(top_bar);
}

/* MENU */

static void hn_menu_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_menu *hn_menu_create(hn_client *client, void *user_data)
{
    return (hn_menu*) hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_MENU,
        sizeof(hn_menu),
        user_data,
        &hn_menu_destroy_handler
    );
}

int hn_menu_set_label(hn_menu *menu, const char *label)
{
    return hn_object_set_label(menu, label);
}

int hn_menu_set_icon(hn_menu *menu, hn_pixel *pixels, u_int32_t width, u_int32_t height)
{
    return hn_object_set_icon(menu, pixels, width, height);
}

int hn_menu_set_enabled(hn_menu *menu, hn_bool enabled)
{
    return hn_object_set_enabled(menu, enabled);
}

int hn_menu_add_to_top_bar(hn_menu *menu, hn_top_bar *top_bar, hn_menu *before)
{
    return hn_object_set_parent(menu, top_bar, before);
}

int hn_menu_add_to_menu(hn_menu *menu, hn_menu *parent, hn_object *before)
{
    return hn_object_set_parent(menu, parent, before);
}

/* ACTION */

static void hn_action_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_action *hn_action_create(hn_client *client, void *user_data)
{
    hn_action *action = hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_ACTION,
        sizeof(hn_action),
        user_data,
        &hn_action_destroy_handler
    );

    return action;
}

int hn_action_set_label(hn_action *action, const char *label)
{
   return hn_object_set_label(action, label);
}

int hn_action_set_icon(hn_action *action, hn_pixel *pixels, u_int32_t width, u_int32_t height)
{
    return hn_object_set_icon(action, pixels, width, height);
}

int hn_action_set_shortcuts(hn_action *action, const char *shortcuts)
{
    return hn_object_set_shortcuts(action, shortcuts);
}

int hn_action_set_enabled(hn_action *action, hn_bool enabled)
{
    return hn_object_set_enabled(action, enabled);
}

int hn_action_add_to_menu(hn_action *action, hn_menu *parent, hn_object *before)
{
    return hn_object_set_parent(action, parent, before);
}

/* TOGGLE */

static void hn_toggle_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_toggle *hn_toggle_create(hn_client *client, void *user_data)
{
    hn_toggle *toggle = hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_TOGGLE,
        sizeof(hn_toggle),
        user_data,
        &hn_toggle_destroy_handler
    );

    return toggle;
}

int hn_toggle_set_label(hn_toggle *toggle, const char *label)
{
    return hn_object_set_label(toggle, label);
}

int hn_toggle_set_enabled(hn_toggle *toggle, hn_bool enabled)
{
    return hn_object_set_enabled(toggle, enabled);
}

int hn_toggle_set_shortcuts(hn_toggle *toggle, const char *shortcuts)
{
    return hn_object_set_shortcuts(toggle, shortcuts);
}

int hn_toggle_set_checked(hn_toggle *toggle, hn_bool checked)
{
    return hn_object_set_checked(toggle, checked);
}

int hn_toggle_add_to_menu(hn_toggle *toggle, hn_menu *parent, hn_object *before)
{
    return hn_object_set_parent(toggle, parent, before);
}

/* SELECT */

static void hn_select_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_select *hn_select_create(hn_client *client, void *user_data)
{
    hn_select *select = (hn_select*) hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_SELECT,
        sizeof(hn_select),
        user_data,
        &hn_select_destroy_handler
    );

    select->active_option = NULL;
    return select;
}

int hn_select_add_to_menu(hn_select *select, hn_menu *parent, hn_object *before)
{
    return hn_object_set_parent(select, parent, before);
}

/* OPTION */

static void hn_option_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_option *hn_option_create(hn_client *client, void *user_data)
{
    return (hn_option*) hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_OPTION,
        sizeof(hn_option),
        user_data,
        &hn_option_destroy_handler
    );
}

int hn_option_set_label(hn_option *option, const char *label)
{
    return hn_object_set_label(option, label);
}

int hn_option_set_enabled(hn_option *option, hn_bool enabled)
{
    return hn_object_set_enabled(option, enabled);
}

int hn_option_set_shortcuts(hn_option *option, const char *shortcuts)
{
    return hn_object_set_shortcuts(option, shortcuts);
}

int hn_option_set_active(hn_option *option)
{
    return hn_object_set_active(option);
}

int hn_option_add_to_select(hn_option *option, hn_select *parent, hn_object *before)
{
    return hn_object_set_parent(option, parent, before);
}

/* SEPARATOR */

static void hn_separator_destroy_handler(hn_object *object)
{
    HN_UNUSED(object);
}

hn_separator *hn_separator_create(hn_client *client, void *user_data)
{
    return (hn_separator*) hn_client_object_create
    (
        client,
        HN_OBJECT_TYPE_SEPARATOR,
        sizeof(hn_separator),
        user_data,
        &hn_separator_destroy_handler
    );
}

int hn_separator_set_label(hn_separator *separator, const char *label)
{
    return hn_object_set_label(separator, label);
}

int hn_separator_add_to_menu(hn_separator *separator, hn_menu *parent, hn_object *before)
{
    return hn_object_set_parent(separator, parent, before);
}

int hn_separator_add_to_select(hn_separator *separator, hn_select *parent, hn_object *before)
{
    return hn_object_set_parent(separator, parent, before);
}

/* EVENTS */

int hn_action_invoke_handler(hn_client *client)
{
    hn_object_id action_id;

    if(hn_server_read(client, &action_id, sizeof(hn_object_id)))
        return HN_CONNECTION_LOST;

    hn_action *action = hn_object_get_by_id(client, action_id);

    if(!action)
    {
        hn_client_destroy(client);
        return HN_CONNECTION_LOST;
    }

    if(action->object.enabled != HN_TRUE)
    {
        hn_client_destroy(client);
        return HN_CONNECTION_LOST;
    }

    client->events_interface->server_action_invoke(action);

    return HN_SUCCESS;
}

int hn_send_custom_event_handler(hn_client *client)
{
    u_int32_t data_len;

    if(hn_server_read(client, &data_len, sizeof(u_int32_t)))
        return HN_CONNECTION_LOST;

    if(data_len == 0)
        return HN_SUCCESS;

    u_int8_t data[data_len];

    if(hn_server_read(client, data, data_len))
        return HN_CONNECTION_LOST;

    client->events_interface->server_send_custom_event(client, data, data_len);

    return HN_SUCCESS;
}

int hn_client_handle_server_event(hn_client *client)
{
    hn_message_id msg_id;

    if(hn_server_read(client, &msg_id, sizeof(hn_message_id)))
        return HN_CONNECTION_LOST;

    switch (msg_id)
    {
        case HN_ACTION_INVOKE_ID:
            return hn_action_invoke_handler(client);
        break;
        case HN_SERVER_TO_CLIENT_SEND_CUSTOM_EVENT_ID:
            return hn_send_custom_event_handler(client);
        break;
    }

    hn_client_destroy(client);
    return HN_CONNECTION_LOST;
}

int hn_client_dispatch_events(hn_client *client, int timeout)
{
    int ret = poll(&client->fd, 1, timeout);

    if(ret == -1)
    {
        hn_client_destroy(client);
        return HN_CONNECTION_LOST;
    }

    if(ret != 0)
    {
        // Disconnected from server
        if(client->fd.revents & POLLHUP)
        {
            hn_client_destroy(client);
            return HN_CONNECTION_LOST;
        }

        // Server event
        if(client->fd.revents & POLLIN)
        {
            return hn_client_handle_server_event(client);
        }
    }

    return HN_SUCCESS;
}



const char *hn_client_get_error_string(hn_client *client)
{
    return client->error;
}
