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
    UInt32 greatest_id;
    pthread_mutex_t mutex;
    hv_top_bar *active_top_bar;
};

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
};

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

    UInt32 type = HV_CONNECTION_TYPE_CLIENT;

    /* CONNECTION TYPE: UInt32 */

    write(client->fd.fd, &type, sizeof(UInt32));

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

    UInt32 msg_id = HV_CLIENT_SET_APP_NAME_ID;
    UInt32 app_name_len = strlen(app_name);

    /* MSG ID: UInt32
     * APP NAME LENGHT: UInt32
     * APP NAME: APP NAME LENGHT */

    write(client->fd.fd, &msg_id, sizeof(UInt32));
    write(client->fd.fd, &app_name_len, sizeof(UInt32));
    write(client->fd.fd, app_name, app_name_len);

    pthread_mutex_unlock(&client->mutex);

    return 0;
}

UInt32 hv_object_new_id(hv_client *client)
{
    UInt32 id;

    if(hv_array_empty(client->free_ids))
    {
        client->greatest_id++;
        id = client->greatest_id;
    }
    else
    {
        id = *(UInt32*)client->free_ids->end->data;
        free(client->free_ids->end->data);
        hv_array_pop_back(client->free_ids);
    }

    return id;
}

void hv_object_free_id(hv_client *client, UInt32 id)
{
    // Free ID
    if(id == client->greatest_id)
        client->greatest_id--;
    else
    {
        UInt32 *freed_id = malloc(sizeof(UInt32));
        *freed_id = id;
        hv_array_push_back(client->free_ids, freed_id);
    }
}

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

hv_object *hv_client_object_create(hv_client *client, UInt32 type, UInt32 size, void *user_data, void (*destroy_func)(hv_object *))
{
    pthread_mutex_lock(&client->mutex);

    UInt32 msg_id = HV_OBJECT_CREATE_ID;

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

    write(client->fd.fd, &msg_id, sizeof(UInt32));
    write(client->fd.fd, &type, sizeof(UInt32));
    write(client->fd.fd, &object->id, sizeof(UInt32));

    pthread_mutex_unlock(&client->mutex);

    return (hv_object*)object;
}

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

int hv_menu_set_title(hv_menu *menu, const char *title)
{
    UInt32 title_len = 0;

    if(title)
        title_len = strlen(title);

    pthread_mutex_lock(&menu->object.client->mutex);

    UInt32 msg_id = HV_MENU_SET_TITLE_ID;

    /* MSG ID: UInt32
     * OBJECT ID: UInt32
     * TITLE LENGHT: UInt32
     * TITLE: TITLE LENGHT */

    write(menu->object.client->fd.fd, &msg_id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &title_len, sizeof(UInt32));

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

    UInt32 before_id = 0;

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

    UInt32 msg_id = HV_MENU_ADD_TO_TOP_BAR_ID;

    /* MSG ID:          UInt32
     * MENU ID:         UInt32
     * MENU BAR ID:     UInt32
     * MENU BEFORE ID:  UInt32 NULLABLE*/

    write(menu->object.client->fd.fd, &msg_id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &top_bar->object.id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &before_id, sizeof(UInt32));

    pthread_mutex_unlock(&menu->object.client->mutex);

    return HV_SUCCESS;
}

int hv_object_destroy(hv_object *obj)
{
    struct hv_object_struct *object = (struct hv_object_struct*)obj;

    UInt32 msg_id = HV_OBJECT_DESTROY_ID;

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

    write(object->client->fd.fd, &msg_id, sizeof(UInt32));
    write(object->client->fd.fd, &object->id, sizeof(UInt32));

    object->destroy_func(object);

    pthread_mutex_unlock(&object->client->mutex);

    free(object);

    return HV_SUCCESS;
}

int hv_top_bar_set_active(hv_top_bar *top_bar)
{
    if(!top_bar)
         return HV_ERROR;

    if(top_bar->object.type != HV_OBJECT_TYPE_TOP_BAR)
        return HV_ERROR;

    if(top_bar->object.client->active_top_bar == top_bar)
        return HV_SUCCESS;

    UInt32 msg_id = HV_TOP_BAR_SET_ACTIVE_ID;

    pthread_mutex_lock(&top_bar->object.client->mutex);

    /* MSG ID:      UInt32
     * TOP BAR ID:  UInt32 */

    write(top_bar->object.client->fd.fd, &msg_id, sizeof(UInt32));
    write(top_bar->object.client->fd.fd, &top_bar->object.id, sizeof(UInt32));

    pthread_mutex_unlock(&top_bar->object.client->mutex);

    return HV_SUCCESS;
}

hv_action *hv_action_create(hv_client *client, void *user_data)
{
    return (hv_action*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_ACTION,
        sizeof(hv_action),
        user_data,
        &hv_action_destroy_handler
    );
}

int hv_action_set_text(hv_action *action, const char *text)
{
    if(!action)
        return HV_ERROR;

    if(action->object.type != HV_OBJECT_TYPE_ACTION)
        return HV_ERROR;

    UInt32 msg_id = HV_ACTION_SET_TEXT_ID;

    UInt32 text_len = 0;

    if(text)
        text_len = strlen(text);


    pthread_mutex_lock(&action->object.client->mutex);

    /* MSG ID:      UInt32
     * ACTION ID:   UInt32
     * TEXT LENGHT: UInt32
     * TEXT:        TEXT LENGHT */

    write(action->object.client->fd.fd, &msg_id, sizeof(UInt32));
    write(action->object.client->fd.fd, &action->object.id, sizeof(UInt32));
    write(action->object.client->fd.fd, &text_len, sizeof(UInt32));

    if(text_len > 0)
        write(action->object.client->fd.fd, text, text_len);

    pthread_mutex_unlock(&action->object.client->mutex);

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

    UInt32 msg_id = HV_MENU_ADD_TO_ACTION_ID;
    write(menu->object.client->fd.fd, &msg_id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &menu->object.id, sizeof(UInt32));
    write(menu->object.client->fd.fd, &action->object.id, sizeof(UInt32));
    pthread_mutex_unlock(&action->object.client->mutex);

    return HV_SUCCESS;
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

    UInt32 msg_id = HV_OBJECT_REMOVE_FROM_PARENT_ID;

    pthread_mutex_lock(&object->client->mutex);

    hv_array_erase(object->parent->children, object->parent_link);
    object->parent_link = NULL;
    object->parent = NULL;

    /* MSG ID:      UInt32
     * OBJECT ID:   UInt32 */

    write(object->client->fd.fd, &msg_id, sizeof(UInt32));
    write(object->client->fd.fd, &object->id, sizeof(UInt32));

    pthread_mutex_unlock(&object->client->mutex);

    return HV_SUCCESS;
}
