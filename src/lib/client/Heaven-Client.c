#include "Heaven-Client.h"
#include <poll.h>
#include <pthread.h>

struct hv_client
{
    struct pollfd fd;
    struct sockaddr_un name;
    struct hv_client_events_interface *events_interface;
    struct hv_array *objects;
    struct hv_array *free_ids;
    UInt32 greatest_id;
    pthread_mutex_t mutex;
};

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




struct hv_client *hv_client_create(const char *socket_name, const char *app_name, struct hv_client_events_interface *events_interface)
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

    UInt32 msg_id = HV_COMMON_IDENTIFY_ID;
    UInt32 type = HV_CONNECTION_TYPE_CLIENT;

    /* MSG ID: UInt32
     * CONNECTION TYPE: UInt32 */

    write(client->fd.fd, &msg_id, sizeof(UInt32));
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

void hv_client_destroy(struct hv_client *client)
{
    // Destroy objects
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

            }break;
            case HV_OBJECT_TYPE_ITEM:
            {

            }break;
            case HV_OBJECT_TYPE_SEPARATOR:
            {

            }break;
        }
    }

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

int hv_client_set_app_name(struct hv_client *client, const char *app_name)
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

UInt32 hv_object_new_id(struct hv_client *client)
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

void hv_object_free_id(struct hv_client *client, UInt32 id)
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

struct hv_object *hv_client_object_create(struct hv_client *client, UInt32 type, UInt32 msg_id, UInt32 size, void *user_data)
{
    pthread_mutex_lock(&client->mutex);

    struct hv_object *object = malloc(size);
    object->type = type;
    object->id = hv_object_new_id(client);
    object->link = hv_array_push_back(client->objects, object);
    object->parent = NULL;
    object->parent_link = NULL;
    object->client = client;
    object->children = hv_array_create();
    object->user_data = user_data;

    /* MSG ID: UInt32
     * MENU BAR ID: UInt32 */

    write(client->fd.fd, &msg_id, sizeof(UInt32));
    write(client->fd.fd, &object->id, sizeof(UInt32));

    pthread_mutex_unlock(&client->mutex);

    return object;
}

void hv_client_object_destroy(struct hv_object *object, UInt32 msg_id)
{

    pthread_mutex_lock(&object->client->mutex);

    if(object->parent)
        hv_array_erase(object->parent->children, object->parent_link);

    // Unlink children
    while(!hv_array_empty(object->children))
    {
        struct hv_object *child = object->children->end->data;
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

    pthread_mutex_unlock(&object->client->mutex);

    free(object);
}

/* MENU BAR */

struct hv_menu_bar *hv_menu_bar_create(struct hv_client *client, void *user_data)
{
    return (struct hv_menu_bar*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_MENU_BAR,
        HV_MENU_BAR_CREATE_ID,
        sizeof(struct hv_menu_bar),
        user_data
    );
}

void hv_menu_bar_destroy(struct hv_menu_bar *menu_bar)
{
    hv_client_object_destroy((struct hv_object *)menu_bar, HV_MENU_BAR_DESTROY_ID);
}

/* MENU */

struct hv_menu *hv_menu_create(struct hv_client *client, void *user_data)
{
    return (struct hv_menu*) hv_client_object_create
    (
        client,
        HV_OBJECT_TYPE_MENU,
        HV_MENU_CREATE_ID,
        sizeof(struct hv_menu),
        user_data
    );
}

int hv_menu_set_title(struct hv_menu *menu, const char *title)
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

void hv_menu_destroy(struct hv_menu *menu)
{
    hv_client_object_destroy((struct hv_object *)menu, HV_MENU_DESTROY_ID);
}
