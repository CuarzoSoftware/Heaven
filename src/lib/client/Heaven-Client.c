#include "Heaven-Client.h"
#include <poll.h>
#include <pthread.h>

struct hv_client_struct
{
    struct pollfd fd;
    struct sockaddr_un name;
    hv_client_events_interface *events_interface;
    hv_array *objects;
    hv_array *free_ids;
    hv_object_id greatest_id;
    pthread_mutex_t mutex;
    hv_top_bar *active_top_bar;
};

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
    void (*destroy_func)(hv_object *);
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

/* CLIENT */

hv_client *hv_client_create(const char *socket_name, const char *app_name, hv_client_events_interface *events_interface)
{
    const char *sock_name;

    if(socket_name)
        sock_name = socket_name;
    else
        sock_name = HV_DEFAULT_SOCKET;

    hv_client *client = malloc(sizeof(hv_client));
    client->events_interface = events_interface;
    client->fd.events = POLLIN;
    client->fd.revents = 0;
    client->active_top_bar = NULL;

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

    memset(&client->name.sun_path, 0, 108);
    client->name.sun_family = AF_UNIX;
    strncpy(client->name.sun_path, sock_name, 107);


    if(connect(client->fd.fd, (const struct sockaddr*)&client->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: No server found in %s.\n", sock_name);
        free(client);
        return NULL;
    }

    /* IDENTIFY */

    pthread_mutex_lock(&client->mutex);

    hv_connection_type type = HV_CONNECTION_TYPE_CLIENT;

    /* CONNECTION TYPE: UInt32 */

    write(client->fd.fd, &type, sizeof(hv_connection_type));

    pthread_mutex_unlock(&client->mutex);

    /* SEND APP NAME */

    if(hv_client_set_app_name(client, app_name))
    {
        printf("Error: App name is NULL.\n");
        close(client->fd.fd);
        free(client);
        return NULL;
    }

    client->objects = hv_array_create();
    client->free_ids = hv_array_create();
    client->greatest_id = 0;

    return client;
}

void hv_client_destroy(hv_client *client)
{
    // Destroy objects
    while(!hv_array_empty(client->objects))
        hv_object_destroy(client->objects->end->data);

    hv_array_destroy(client->objects);

    // Free freed IDs
    while(!hv_array_empty(client->free_ids))
    {
        free(client->free_ids->end->data);
        hv_array_pop_back(client->free_ids);
    }

    hv_array_destroy(client->free_ids);

    pthread_mutex_destroy(&client->mutex);

    close(client->fd.fd);

    free(client);
}

int hv_client_set_app_name(hv_client *client, const char *app_name)
{
    if(app_name == NULL)
        return 1;

    pthread_mutex_lock(&client->mutex);

    hv_message_id msg_id = HV_CLIENT_SET_APP_NAME_ID;
    hv_string_length app_name_len;
    u_int32_t app_name_len_32 = strlen(app_name);

    if(app_name_len_32 > 255)
        app_name_len = 255;
    else
        app_name_len = app_name_len_32;

    /* MSG ID: UInt32
     * APP NAME LENGHT: UInt32
     * APP NAME: APP NAME LENGHT */

    write(client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(client->fd.fd, &app_name_len, sizeof(hv_string_length));
    write(client->fd.fd, app_name, app_name_len);

    pthread_mutex_unlock(&client->mutex);

    return 0;
}

int hv_client_get_fd(hv_client *client)
{
    return client->fd.fd;
}

/* UTILS */

hv_object_id hv_object_new_id(hv_client *client)
{
    hv_object_id id;

    if(hv_array_empty(client->free_ids))
    {
        client->greatest_id++;
        id = client->greatest_id;
    }
    else
    {
        id = *(hv_object_id*)client->free_ids->end->data;
        free(client->free_ids->end->data);
        hv_array_pop_back(client->free_ids);
    }

    return id;
}

void hv_object_free_id(hv_client *client, hv_object_id id)
{
    // Free ID
    if(id == client->greatest_id)
        client->greatest_id--;
    else
    {
        hv_object_id *freed_id = malloc(sizeof(hv_object_id));
        *freed_id = id;
        hv_array_push_back(client->free_ids, freed_id);
    }
}

hv_object *hv_client_object_create(hv_client *client, hv_object_type type, u_int32_t size, void *user_data, void (*destroy_func)(hv_object *))
{
    pthread_mutex_lock(&client->mutex);

    hv_message_id msg_id = HV_OBJECT_CREATE_ID;

    struct hv_object_struct *object = malloc(size);
    object->type = type;
    object->id = hv_object_new_id(client);
    object->link = hv_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hv_array_create();
    object->user_data = user_data;
    object->destroy_func = destroy_func;

    /* MSG ID: UInt32
     * OBJECT TYPE ID: UInt32
     * MENU BAR ID: UInt32 */

    write(client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(client->fd.fd, &type, sizeof(hv_object_type));
    write(client->fd.fd, &object->id, sizeof(hv_object_id));

    pthread_mutex_unlock(&client->mutex);

    return (hv_object*)object;
}

int hv_object_remove_from_parent(hv_object *obj)
{
    struct hv_object_struct *object = (struct hv_object_struct *)obj;

    if(!object)
        return HV_ERROR;

    if(object->type != HV_OBJECT_TYPE_MENU && object->type != HV_OBJECT_TYPE_ACTION && object->type != HV_OBJECT_TYPE_SEPARATOR)
        return HV_ERROR;

    if(!object->parent)
        return HV_SUCCESS;

    hv_message_id msg_id = HV_OBJECT_REMOVE_FROM_PARENT_ID;

    pthread_mutex_lock(&object->client->mutex);

    hv_array_erase(object->parent->children, object->parent_link);
    object->parent_link = NULL;
    object->parent = NULL;

    /* MSG ID:      UInt32
     * OBJECT ID:   UInt32 */

    write(object->client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(object->client->fd.fd, &object->id, sizeof(hv_object_id));

    pthread_mutex_unlock(&object->client->mutex);

    return HV_SUCCESS;
}

/* OBJECTS DESTRUCTORS */

static void hv_top_bar_destroy_handler(hv_object *object)
{
    hv_top_bar *top_bar = (hv_top_bar*)object;

    if(top_bar == top_bar->object.client->active_top_bar)
    {
        hv_node *node = top_bar->object.client->objects->end;

        while(node)
        {
            struct hv_object_struct *obj = node->data;

            if(obj->type == HV_OBJECT_TYPE_TOP_BAR)
            {
                top_bar->object.client->active_top_bar = (hv_top_bar*)obj;
                return;
            }

            node = node->prev;
        }

        top_bar->object.client->active_top_bar = NULL;
    }

}

static void hv_menu_destroy_handler(hv_object *object)
{

}

static void hv_action_destroy_handler(hv_object *object)
{

}

static void hv_separator_destroy_handler(hv_object *object)
{

}

int hv_object_destroy(hv_object *obj)
{
    struct hv_object_struct *object = (struct hv_object_struct*)obj;

    hv_message_id msg_id = HV_OBJECT_DESTROY_ID;

    pthread_mutex_lock(&object->client->mutex);

    if(object->parent)
        hv_array_erase(object->parent->children, object->parent_link);

    // Unlink children
    while(!hv_array_empty(object->children))
    {
        struct hv_object_struct *child = object->children->end->data;
        child->parent_link = NULL;
        child->parent = NULL;
        hv_array_pop_back(object->children);
    }

    hv_array_destroy(object->children);

    hv_object_free_id(object->client, object->id);

    hv_array_erase(object->client->objects, object->link);

    /* MSG ID: UInt32
     * OBJECT ID: UInt32 */

    write(object->client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(object->client->fd.fd, &object->id, sizeof(hv_object_id));

    object->destroy_func(object);

    pthread_mutex_unlock(&object->client->mutex);

    free(object);

    return HV_SUCCESS;
}

/* OBJECTS CONSTRUCTORS */

hv_top_bar *hv_top_bar_create(hv_client *client, void *user_data)
{
    hv_top_bar *top_bar = (hv_top_bar*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_TOP_BAR,
        sizeof(hv_top_bar),
        user_data,
        &hv_top_bar_destroy_handler
    );

    if(client->active_top_bar == NULL)
    {
        client->active_top_bar = top_bar;
        hv_top_bar_set_active(top_bar);
    }

    return top_bar;
}

hv_menu *hv_menu_create(hv_client *client, void *user_data)
{
    return (hv_menu*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_MENU,
        sizeof(hv_menu),
        user_data,
        &hv_menu_destroy_handler
    );
}

hv_action *hv_action_create(hv_client *client, void *user_data)
{
    hv_action *action = hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_ACTION,
        sizeof(hv_action),
        user_data,
        &hv_action_destroy_handler
    );

    action->state = HV_ACTION_STATE_ENABLED;

    return action;
}

hv_separator *hv_separator_create(hv_client *client, void *user_data)
{
    return (hv_separator*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_SEPARATOR,
        sizeof(hv_separator),
        user_data,
        &hv_separator_destroy_handler
    );
}

hv_object *hv_object_create(hv_client *client, hv_object_type type, void *user_data)
{
    switch (type)
    {
        case HV_OBJECT_TYPE_TOP_BAR:
            return hv_top_bar_create(client, user_data);
            break;
        case HV_OBJECT_TYPE_MENU:
            return hv_menu_create(client, user_data);
            break;
        case HV_OBJECT_TYPE_ACTION:
            return hv_action_create(client, user_data);
            break;
        case HV_OBJECT_TYPE_SEPARATOR:
            return hv_separator_create(client, user_data);
            break;
    }

    return NULL;
}

/* TOP BAR */

int hv_top_bar_set_active(hv_top_bar *top_bar)
{
    if(!top_bar)
         return HV_ERROR;

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
        return HV_ERROR;

    if(top_bar->object.client->active_top_bar == top_bar)
        return HV_SUCCESS;

    hv_message_id msg_id = HV_TOP_BAR_SET_ACTIVE_ID;

    pthread_mutex_lock(&top_bar->object.client->mutex);

    /* MSG ID:      UInt32
     * TOP BAR ID:  UInt32 */

    write(top_bar->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(top_bar->object.client->fd.fd, &top_bar->object.id, sizeof(hv_object_id));

    pthread_mutex_unlock(&top_bar->object.client->mutex);

    return HV_SUCCESS;
}

/* MENU */

int hv_menu_set_title(hv_menu *menu, const char *title)
{
    hv_string_length title_len = 0;

    if(title)
    {
        u_int32_t title_len_32 = strlen(title);

        if(title_len_32 > 255)
            title_len = 255;
        else
            title_len = title_len_32;
    }

    pthread_mutex_lock(&menu->object.client->mutex);

    hv_message_id msg_id = HV_MENU_SET_TITLE_ID;

    /* MSG ID: UInt32
     * OBJECT ID: UInt32
     * TITLE LENGHT: UInt32
     * TITLE: TITLE LENGHT */

    write(menu->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &title_len, sizeof(hv_string_length));

    if(title_len > 0)
        write(menu->object.client->fd.fd, title, title_len);

    pthread_mutex_unlock(&menu->object.client->mutex);

    return 0;
}

int hv_menu_add_to_top_bar(hv_menu *menu, hv_top_bar *top_bar, hv_menu *before)
{
    if(!menu || !top_bar)
         return HV_ERROR;
    else
    {
        if(menu->object.type != HV_OBJECT_TYPE_MENU)
            return HV_ERROR;

        if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
            return HV_ERROR;
    }

    if(menu->object.client != top_bar->object.client)
        return HV_ERROR;

    pthread_mutex_lock(&menu->object.client->mutex);

    hv_object_id before_id = 0;

    if(before)
    {
        if(before->object.type != HV_OBJECT_TYPE_MENU || before->object.client != menu->object.client || (hv_top_bar *)before->object.parent != top_bar)
        {
            pthread_mutex_unlock(&menu->object.client->mutex);
            return HV_ERROR;
        }

        before_id = before->object.id;

        // Remove from current parent
        if(menu->object.parent)
            hv_array_erase(menu->object.parent->children, menu->object.parent_link);

        // Set parent
        menu->object.parent = (hv_object *)top_bar;

        // Insert before before
        menu->object.parent_link = hv_array_insert_before(menu->object.parent->children, before->object.parent_link, menu);

    }
    else
    {
        // Remove from current parent
        if(menu->object.parent)
            hv_array_erase(menu->object.parent->children, menu->object.parent_link);

        // Set parent
        menu->object.parent = (hv_object *)top_bar;

        // Push back
        menu->object.parent_link = hv_array_push_back(menu->object.parent->children, menu);
    }

    hv_message_id msg_id = HV_MENU_ADD_TO_TOP_BAR_ID;

    /* MSG ID:          UInt32
     * MENU ID:         UInt32
     * MENU BAR ID:     UInt32
     * MENU BEFORE ID:  UInt32 NULLABLE*/

    write(menu->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &top_bar->object.id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &before_id, sizeof(hv_object_id));

    pthread_mutex_unlock(&menu->object.client->mutex);

    return HV_SUCCESS;
}

int hv_menu_add_to_action(hv_menu *menu, hv_action *action)
{
    if(!menu || !action)
        return HV_ERROR;

    if(menu->object.type != HV_OBJECT_TYPE_MENU || action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    if(menu->object.client != action->object.client)
        return HV_ERROR;

    if((hv_object*)menu == action->object.parent)
        return HV_ERROR;

    pthread_mutex_lock(&action->object.client->mutex);

    if(menu->object.parent)
        hv_array_erase(menu->object.parent->children, menu->object.parent_link);

    if(!hv_array_empty(action->object.children))
    {
        hv_menu *child = (hv_menu*)action->object.children->end->data;
        child->object.parent = NULL;
        child->object.parent_link = NULL;
        hv_array_pop_back(action->object.children);
    }

    menu->object.parent = (struct hv_object_struct*)action;
    menu->object.parent_link = hv_array_push_back(action->object.children, menu);

    /* MSG ID:    UInt32
     * MENU ID:   UInt32
     * ACTION ID: UInt32*/

    hv_message_id msg_id = HV_MENU_ADD_TO_ACTION_ID;
    write(menu->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &action->object.id, sizeof(hv_object_id));
    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;
}

/* ACTION */

int hv_action_set_text(hv_action *action, const char *text)
{
    if(!action)
        return HV_ERROR;

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    hv_message_id msg_id = HV_ACTION_SET_TEXT_ID;

    hv_string_length text_len = 0;

    if(text)
    {
        u_int32_t text_len_32 = strlen(text);

        if(text_len_32 > 255)
            text_len = 255;
        else
            text_len = text_len_32;
    }


    pthread_mutex_lock(&action->object.client->mutex);

    /* MSG ID:      UInt32
     * ACTION ID:   UInt32
     * TEXT LENGHT: UInt32
     * TEXT:        TEXT LENGHT */

    write(action->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(action->object.client->fd.fd, &action->object.id, sizeof(hv_object_id));
    write(action->object.client->fd.fd, &text_len, sizeof(hv_string_length));

    if(text_len > 0)
        write(action->object.client->fd.fd, text, text_len);

    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;
}

int hv_action_set_icon(hv_action *action, hv_pixel *pixels, u_int32_t width, u_int32_t height)
{
    if(!action)
        return HV_ERROR;

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    u_int32_t total_pixels = width*height;

    if(pixels)
    {
        if(total_pixels == 0)
            return HV_ERROR;

        // Memory access test
        hv_pixel test_pixel = pixels[total_pixels-1];
        (void)test_pixel;
    }

    hv_message_id msg_id = HV_ACTION_SET_ICON_ID;

    pthread_mutex_lock(&action->object.client->mutex);

    write(action->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(action->object.client->fd.fd, &action->object.id, sizeof(hv_object_id));

    if(!pixels)
    {
        width = 0;
        write(action->object.client->fd.fd, &width, sizeof(u_int32_t));
    }
    else
    {
        write(action->object.client->fd.fd, &width, sizeof(u_int32_t));
        write(action->object.client->fd.fd, &height, sizeof(u_int32_t));
        write(action->object.client->fd.fd, pixels, total_pixels);
    }

    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;

}

int hv_action_set_shortcuts(hv_action *action, const char *shortcuts)
{
    if(!action)
        return HV_ERROR;

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    hv_string_length shortcuts_len = 0;

    if(shortcuts)
    {
        u_int32_t shortcuts_len_32 = 0;
        shortcuts_len_32 = strlen(shortcuts);

        if(shortcuts_len_32 > 255)
            shortcuts_len = 255;
        else
            shortcuts_len = shortcuts_len_32;
    }

    hv_message_id msg_id = HV_ACTION_SET_SHORTCUTS_ID;

    pthread_mutex_lock(&action->object.client->mutex);

    write(action->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(action->object.client->fd.fd, &action->object.id, sizeof(hv_object_id));
    write(action->object.client->fd.fd, &shortcuts_len, sizeof(hv_string_length));

    if(shortcuts)
        write(action->object.client->fd.fd, shortcuts, shortcuts_len);

    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;
}

int hv_action_set_state(hv_action *action, hv_action_state state)
{
    if(!action)
        return HV_ERROR;

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    if(state != HV_ACTION_STATE_ENABLED && state != HV_ACTION_STATE_DISABLED)
        return HV_ERROR;

    pthread_mutex_lock(&action->object.client->mutex);

    if(state == action->state)
    {
        pthread_mutex_unlock(&action->object.client->mutex);
        return HV_SUCCESS;
    }

    hv_message_id msg_id = HV_ACTION_SET_STATE_ID;

    action->state = state;

    write(action->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(action->object.client->fd.fd, &action->object.id, sizeof(hv_object_id));
    write(action->object.client->fd.fd, &state, sizeof(hv_action_state));

    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;
}

/* SEPARATOR */

int hv_separator_set_text(hv_separator *separator, const char *text)
{
    if(!separator)
        return HV_ERROR;

    if(separator->object.type != HV_OBJECT_TYPE_SEPARATOR)
        return HV_ERROR;

    hv_message_id msg_id = HV_SEPARATOR_SET_TEXT_ID;

    hv_string_length text_len = 0;

    if(text)
    {
        u_int32_t text_len_32 = strlen(text);

        if(text_len_32 > 255)
            text_len = 255;
        else
            text_len = text_len_32;
    }

    pthread_mutex_lock(&separator->object.client->mutex);

    /* MSG ID:      UInt32
     * ACTION ID:   UInt32
     * TEXT LENGHT: UInt32
     * TEXT:        TEXT LENGHT */

    write(separator->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(separator->object.client->fd.fd, &separator->object.id, sizeof(hv_object_id));
    write(separator->object.client->fd.fd, &text_len, sizeof(hv_string_length));

    if(text_len > 0)
        write(separator->object.client->fd.fd, text, text_len);

    pthread_mutex_unlock(&separator->object.client->mutex);

    return HV_SUCCESS;
}

/* ITEM */

int hv_item_add_to_menu(hv_item *_item, hv_menu *menu, hv_item *_before)
{
    struct hv_object_struct *item = (struct hv_object_struct*)_item;
    struct hv_object_struct *before = (struct hv_object_struct*)_before;

    if(!_item || !menu)
         return HV_ERROR;
    else
    {
        if(item->type != HV_OBJECT_TYPE_ACTION && item->type != HV_OBJECT_TYPE_SEPARATOR)
            return HV_ERROR;

        if(menu->object.type != HV_OBJECT_TYPE_MENU)
            return HV_ERROR;
    }

    if(menu->object.client != item->client)
        return HV_ERROR;

    if(menu->object.parent == item)
        return HV_ERROR;

    pthread_mutex_lock(&menu->object.client->mutex);

    hv_object_id before_id = 0;

    if(before)
    {
        if((before->type != HV_OBJECT_TYPE_ACTION && before->type != HV_OBJECT_TYPE_SEPARATOR) || before->client != menu->object.client || (hv_object*)before->parent != menu)
        {
            pthread_mutex_unlock(&menu->object.client->mutex);
            return HV_ERROR;
        }

        before_id = before->id;

        // Remove from current parent
        if(item->parent)
            hv_array_erase(item->parent->children, item->parent_link);

        // Set parent
        item->parent = (hv_object *)menu;

        // Insert before before
        item->parent_link = hv_array_insert_before(menu->object.children, before->parent_link, item);

    }
    else
    {
        // Remove from current parent
        if(item->parent)
            hv_array_erase(item->parent->children, item->parent_link);

        // Set parent
        item->parent = (hv_object *)menu;

        // Push back
        item->parent_link = hv_array_push_back(menu->object.children, menu);
    }

    hv_message_id msg_id = HV_ITEM_ADD_TO_MENU_ID;

    /* MSG ID:          UInt32
     * ITEM ID:         UInt32
     * MENU ID:         UInt32
     * ITEM BEFORE ID:  UInt32 NULLABLE*/

    write(menu->object.client->fd.fd, &msg_id, sizeof(hv_message_id));
    write(menu->object.client->fd.fd, &item->id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(hv_object_id));
    write(menu->object.client->fd.fd, &before_id, sizeof(hv_object_id));

    pthread_mutex_unlock(&menu->object.client->mutex);

    return HV_SUCCESS;
}
