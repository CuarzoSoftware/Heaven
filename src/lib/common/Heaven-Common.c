#include "Heaven-Common.h"

struct hv_object
{
    UInt32 type;
    UInt32 id;
};

struct hv_array *hv_array_create()
{
    struct hv_array *array = malloc(sizeof(struct hv_array));
    array->begin = NULL;
    array->end = NULL;
    return array;
}

void hv_array_destroy(struct hv_array *array)
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

void hv_array_pop_front(struct hv_array *array)
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

    struct hv_node *next = array->begin->next;
    next->prev = NULL;
    free(array->begin);
    array->begin = next;
}

void hv_array_pop_back(struct hv_array *array)
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

    struct hv_node *prev = array->end->prev;
    prev->next = NULL;
    free(array->end);
    array->end = prev;
}

int hv_array_empty(struct hv_array *array)
{
    if(array->end != NULL)
        return 0;
    return 1;
}

struct hv_node *hv_array_push_back(struct hv_array *array, void *data)
{
    struct hv_node *node = malloc(sizeof(struct hv_node));
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

void hv_array_erase(struct hv_array *array, struct hv_node *node)
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


UInt32 hv_object_get_id(struct hv_object *object)
{
    return object->id;
}
