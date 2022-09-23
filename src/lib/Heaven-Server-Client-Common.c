#include "Heaven-Server-Client-Common.h"

struct hn_object_struct
{
    void *user_data;
    hn_object_type type;
    hn_object_id id;
    hn_client *client;
    hn_array *children;
    hn_object *parent;
    hn_node *parent_link;
};

struct hn_client_struct
{
    hn_array *objects;
    void *user_data;
};

hn_object *hn_object_get_by_id(hn_client *client, hn_object_id id)
{
    hn_node *node = client->objects->begin;

    while(node)
    {
        struct hn_object_struct *object = node->data;

        if(object->id == id)
            return object;

        node = node->next;
    }

    return NULL;
}

hn_object_id hn_object_get_id(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->id;
}

hn_object_type hn_object_get_type(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->type;
}

hn_object *hn_object_get_parent(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->parent;
}

hn_client *hn_object_get_client(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->client;
}

void *hn_object_get_user_data(hn_object *object)
{
    struct hn_object_struct *obj = object;
    return obj->user_data;
}

void hn_object_set_user_data(hn_object *object, void *user_data)
{
    struct hn_object_struct *obj = object;
    obj->user_data = user_data;
}

/*!
 * \brief hn_client_get_user_data
 * \param client
 * \return
 */
void *hn_client_get_user_data(hn_client *client)
{
    return client->user_data;
}

void hn_client_set_user_data(hn_client *client, void *user_data)
{
    client->user_data = user_data;
}

const hn_array *hn_object_get_children(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->children;
}

const hn_node *hn_object_get_parent_link(hn_object *object)
{
    struct hn_object_struct *obj = (struct hn_object_struct *)object;
    return obj->parent_link;
}
