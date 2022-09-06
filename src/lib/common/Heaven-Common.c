#include "Heaven-Common.h"


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
    if(array->end)
        return 0;
    return 1;
}
