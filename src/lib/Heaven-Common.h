#ifndef HEAVENCOMMON_H
#define HEAVENCOMMON_H

/*!
 * @defgroup Common
 *
 * @brief Shared members.
 *
 * @addtogroup Common
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define HV_DEFAULT_SOCKET "heaven-0"

#define HV_UNUSED(variable)(void)variable;

typedef u_int8_t hv_connection_type;
typedef u_int8_t hv_message_id;
typedef u_int8_t hv_string_length;

/* CONNECTION TYPES */

enum HV_CONNECTION_TYPE
{
    HV_CONNECTION_TYPE_COMPOSITOR = 1,
    HV_CONNECTION_TYPE_CLIENT = 2
};

/*!
 * @brief Method return values.
 */
enum HV_RETURN_CODE
{
    /*! Method call executed successfully. */
    HV_SUCCESS = 1,

    /*! Method call failed due to invalid agruments. */
    HV_ERROR = 0,

    /*! Connection with the other end is lost. */
    HV_CONNECTION_LOST = -1
};


/* Array */

typedef struct hv_node_struct hv_node;

struct hv_node_struct
{
    hv_node *prev;
    hv_node *next;
    void *data;
};

typedef struct hv_array_struct hv_array;

struct hv_array_struct
{
    hv_node *begin;
    hv_node *end;
};

hv_array *hv_array_create();
void hv_array_destroy(hv_array *array);
void hv_array_pop_front(hv_array *array);
void hv_array_pop_back(hv_array *array);
int hv_array_empty(hv_array *array);
void hv_array_erase(hv_array *array, hv_node *node);
hv_node *hv_array_push_back(hv_array *array, void *data);
hv_node *hv_array_push_front(hv_array *array, void *data);
hv_node *hv_array_insert_before(hv_array *array, hv_node *before, void *data);

/*! @} */

#endif // HEAVENCOMMON_H
