#include "Heaven-Common.h"

hn_array *hn_array_create()
{
    hn_array *array = malloc(sizeof(hn_array));
    array->begin = NULL;
    array->end = NULL;
    return array;
}

void hn_array_destroy(hn_array *array)
{
    if(hn_array_empty(array))
    {
        free(array);
        return;
    }

    while(!hn_array_empty(array))
        hn_array_pop_back(array);

    free(array);
}

void hn_array_pop_front(hn_array *array)
{
    if(hn_array_empty(array))
        return;

    // If only 1 element
    if(!array->begin->next)
    {
        free(array->begin);
        array->begin = NULL;
        array->end = NULL;
        return;
    }

    hn_node *next = array->begin->next;
    next->prev = NULL;
    free(array->begin);
    array->begin = next;
}

void hn_array_pop_back(hn_array *array)
{
    if(hn_array_empty(array))
        return;

    // If only 1 element
    if(!array->end->prev)
    {
        free(array->end);
        array->begin = NULL;
        array->end = NULL;
        return;
    }

    hn_node *prev = array->end->prev;
    prev->next = NULL;
    free(array->end);
    array->end = prev;
}

int hn_array_empty(hn_array *array)
{
    if(array->end != NULL)
        return 0;
    return 1;
}

hn_node *hn_array_push_back(hn_array *array, void *data)
{
    hn_node *node = malloc(sizeof(hn_node));
    node->data = data;
    node->next = NULL;

    if(hn_array_empty(array))
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

hn_node *hn_array_push_front(hn_array *array, void *data)
{
    hn_node *node = malloc(sizeof(hn_node));
    node->data = data;
    node->prev = NULL;

    if(hn_array_empty(array))
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

hn_node *hn_array_insert_before(hn_array *array, hn_node *before, void *data)
{
    if(hn_array_empty(array) || array->begin == before)
        return hn_array_push_front(array, data);

    hn_node *node = malloc(sizeof(hn_node));
    node->data = data;
    node->prev = before->prev;
    before->prev->next = node;
    node->next = before;
    before->prev = node;
    return node;
}

void hn_array_erase(hn_array *array, hn_node *node)
{
    if(hn_array_empty(array))
        return;

    if(array->begin == node)
    {
        hn_array_pop_front(array);
        return;
    }

    if(array->end == node)
    {
        hn_array_pop_back(array);
        return;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
}


void hn_debug(const char *msg)
{
    printf("%s\n",msg);
}
