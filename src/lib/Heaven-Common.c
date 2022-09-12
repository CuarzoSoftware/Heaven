#include "Heaven-Common.h"

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

hv_array *hv_array_create()
{
    hv_array *array = malloc(sizeof(hv_array));
    array->begin = NULL;
    array->end = NULL;
    return array;
}

void hv_array_destroy(hv_array *array)
{
    if(hv_array_empty(array))
    {
        free(array);
        return;
    }

    while(!hv_array_empty(array))
        hv_array_pop_back(array);

    free(array);
}

void hv_array_pop_front(hv_array *array)
{
    if(hv_array_empty(array))
        return;

    // If only 1 element
    if(!array->begin->next)
    {
        free(array->begin);
        array->begin = NULL;
        array->end = NULL;
        return;
    }

    hv_node *next = array->begin->next;
    next->prev = NULL;
    free(array->begin);
    array->begin = next;
}

void hv_array_pop_back(hv_array *array)
{
    if(hv_array_empty(array))
        return;

    // If only 1 element
    if(!array->end->prev)
    {
        free(array->end);
        array->begin = NULL;
        array->end = NULL;
        return;
    }

    hv_node *prev = array->end->prev;
    prev->next = NULL;
    free(array->end);
    array->end = prev;
}

int hv_array_empty(hv_array *array)
{
    if(array->end != NULL)
        return 0;
    return 1;
}

hv_node *hv_array_push_back(hv_array *array, void *data)
{
    hv_node *node = malloc(sizeof(hv_node));
    node->data = data;
    node->next = NULL;

    if(hv_array_empty(array))
    {
        node->prev = NULL;
        array->begin = node;
        array->end = node;
        return node;
    }

    node->prev = array->end;
    array->end->next = node;
    array->end = node;
    return node;
}

hv_node *hv_array_push_front(hv_array *array, void *data)
{
    hv_node *node = malloc(sizeof(hv_node));
    node->data = data;
    node->prev = NULL;

    if(hv_array_empty(array))
    {
        node->next = NULL;
        array->begin = node;
        array->end = node;
        return node;
    }

    node->next = array->begin;
    array->begin->prev = node;
    array->begin = node;
    return node;
}

hv_node *hv_array_insert_before(hv_array *array, hv_node *before, void *data)
{
    if(hv_array_empty(array) || array->begin == before)
        return hv_array_push_front(array, data);

    hv_node *node = malloc(sizeof(hv_node));
    node->data = data;
    node->prev = before->prev;
    node->next = before;
    before->prev = node;
    return node;
}

void hv_array_erase(hv_array *array, hv_node *node)
{
    if(hv_array_empty(array))
        return;

    if(array->begin == node)
    {
        hv_array_pop_front(array);
        return;
    }

    if(array->end == node)
    {
        hv_array_pop_back(array);
        return;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
}

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

void *hv_client_get_user_data(hv_client *client)
{
    return client->user_data;
}

void hv_client_set_user_data(hv_client *client, void *user_data)
{
    client->user_data = user_data;
}
