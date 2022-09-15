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

#ifdef __cplusplus
extern "C" {
#endif

#define HN_DEFAULT_SOCKET "heaven-0"

#define HN_UNUSED(variable)(void)variable;

typedef u_int8_t hn_connection_type;
typedef u_int8_t hn_message_id;
typedef u_int8_t hn_string_length;

/*! A value that can be either TRUE or FALSE. */
typedef u_int8_t hn_bool;

/* CONNECTION TYPES */

enum HN_CONNECTION_TYPE
{
    HN_CONNECTION_TYPE_COMPOSITOR = 1,
    HN_CONNECTION_TYPE_CLIENT = 2
};

/*!
 * @brief Method return values.
 */
enum HN_RETURN_CODE
{
    /*! Method call executed successfully. */
    HN_SUCCESS = 1,

    /*! Method call failed due to invalid agruments. */
    HN_ERROR = 0,

    /*! Connection with the other end is lost. */
    HN_CONNECTION_LOST = -1
};

enum HN_BOOL
{
    HN_FALSE = 0,
    HN_TRUE = 1,
};


/* Array */

typedef struct hn_node_struct hn_node;

struct hn_node_struct
{
    hn_node *prev;
    hn_node *next;
    void *data;
};

typedef struct hn_array_struct hn_array;

struct hn_array_struct
{
    hn_node *begin;
    hn_node *end;
};

hn_array *hn_array_create();
void hn_array_destroy(hn_array *array);
void hn_array_pop_front(hn_array *array);
void hn_array_pop_back(hn_array *array);
int hn_array_empty(hn_array *array);
void hn_array_erase(hn_array *array, hn_node *node);
hn_node *hn_array_push_back(hn_array *array, void *data);
hn_node *hn_array_push_front(hn_array *array, void *data);
hn_node *hn_array_insert_before(hn_array *array, hn_node *before, void *data);

/*! @} */

#ifdef __cplusplus
}
#endif

#endif // HEAVENCOMMON_H
