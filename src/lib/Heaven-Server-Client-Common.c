#include "Heaven-Server-Client-Common.h"

struct hv_object_struct
{
    hv_object_type type;
    hv_object_id id;
    hv_client *client;
    hv_object *parent;
};

struct hv_client_struct
{
    hv_array *objects;
    void *user_data;
};

hv_object *hv_object_get_by_id(hv_client *client, hv_object_id id)
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

hv_object_id hv_object_get_id(hv_object *object)
{
    struct hv_object_struct *obj = (struct hv_object_struct *)object;
    return obj->id;
}

hv_object_type hv_object_get_type(hv_object *object)
{
    struct hv_object_struct *obj = (struct hv_object_struct *)object;
    return obj->type;
}

hv_object *hv_object_get_parent(hv_object *object)
{
    struct hv_object_struct *obj = (struct hv_object_struct *)object;
    return obj->parent;
}

hv_client *hv_object_get_client(hv_object *object)
{
    struct hv_object_struct *obj = (struct hv_object_struct *)object;
    return obj->client;
}

/*!
 * \brief hv_client_get_user_data
 * \param client
 * \return
 */
void *hv_client_get_user_data(hv_client *client)
{
    return client->user_data;
}

void hv_client_set_user_data(hv_client *client, void *user_data)
{
    client->user_data = user_data;
}
