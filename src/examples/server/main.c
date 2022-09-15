#include "../../lib/Heaven-Server.h"
#include <poll.h>

const char *object_type_to_string(hn_object_type object_type)
{
    switch (object_type)
    {
        case HN_OBJECT_TYPE_TOP_BAR:
            return "TOP BAR";
            break;
        case HN_OBJECT_TYPE_MENU:
            return "MENU";
            break;
        case HN_OBJECT_TYPE_ACTION:
            return "ACTION";
            break;
        case HN_OBJECT_TYPE_TOGGLE:
            return "TOGGLE";
            break;
        case HN_OBJECT_TYPE_SELECT:
            return "SELECT";
            break;
        case HN_OBJECT_TYPE_OPTION:
            return "OPTION";
            break;
        case HN_OBJECT_TYPE_SEPARATOR:
            return "SEPARATOR";
            break;
    }

    return NULL;
}

const char *bool_to_string(hn_bool checked)
{
    if(checked)
        return "TRUE";
    return "FALSE";
}


static void client_connected(hn_client *client)
{
    pid_t pid;
    hn_client_get_credentials(client, &pid, NULL, NULL);
    printf("- CLIENT (%d) WITH PID (%d) CONNECTED\n",
           hn_client_get_id(client),
           pid);
}

static void client_set_app_name(hn_client *client, const char *app_name)
{
    printf("- CLIENT (%d) SET APP NAME = \"%s\"\n",
           hn_client_get_id(client),
           app_name);
}

static void client_send_custom_request(hn_client *client, void *data, u_int32_t size)
{
    char *msg = data;
    msg[size] = '\0';
    printf("- CLIENT (%d) SENT A CUSTOM MESSAGE = \"%s\"\n",
           hn_client_get_id(client),
           msg);

    char *reply = "Hello my dear client! <3";
    hn_server_send_custom_event_to_client(client, reply, strlen(reply) + 1);
}

static void client_disconnected(hn_client *client)
{
    pid_t pid;
    hn_client_get_credentials(client, &pid, NULL, NULL);
    printf("- CLIENT (%d) WITH PID (%d) DISCONNECTED\n",
           hn_client_get_id(client),
           pid);
}


static void object_create(hn_client *client, hn_object *object)
{
    printf("- CLIENT (%d) CREATED %s (%d)\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object));
}

static void object_set_parent(hn_client *client, hn_object *object, hn_object *parent, hn_object *before)
{
    if(parent)
    {
        if(before)
        {
            printf("- CLIENT (%d) ADDS %s (%d) TO %s (%d) BEFORE %s (%d)\n",
                   hn_client_get_id(client),
                   object_type_to_string(hn_object_get_type(object)),
                   hn_object_get_id(object),
                   object_type_to_string(hn_object_get_type(parent)),
                   hn_object_get_id(parent),
                   object_type_to_string(hn_object_get_type(before)),
                   hn_object_get_id(before));
        }
        else
        {
            printf("- CLIENT (%d) ADDS %s (%d) TO THE END OF %s (%d)\n",
                   hn_client_get_id(client),
                   object_type_to_string(hn_object_get_type(object)),
                   hn_object_get_id(object),
                   object_type_to_string(hn_object_get_type(parent)),
                   hn_object_get_id(parent));
        }
    }
    else
    {
        hn_object *current_parent = hn_object_get_parent(object);

        printf("- CLIENT (%d) REMOVES %s (%d) FROM PARENT %s (%d)\n",
               hn_client_get_id(client),
               object_type_to_string(hn_object_get_type(object)),
               hn_object_get_id(object),
               object_type_to_string(hn_object_get_type(current_parent)),
               hn_object_get_id(current_parent));
    }
}

static void object_set_label(hn_client *client, hn_object *object, const char *label)
{
    hn_client_id client_id = hn_client_get_id(client);

    printf("- CLIENT (%d) SET %s (%d) LABEL TO \"%s\"\n",
           client_id,
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object),
           label);
}

static void object_set_icon(hn_client *client, hn_object *object, const hn_pixel *pixels, u_int32_t width, u_int32_t height)
{
    if(pixels)
        printf("- CLIENT (%d) SET %s (%d) ICON %dx%d\n",
               hn_client_get_id(client),
               object_type_to_string(hn_object_get_type(object)),
               hn_object_get_id(object),
               width,
               height);
    else
        printf("- CLIENT (%d) REMOVED %s (%d) ICON\n",
               hn_client_get_id(client),
               object_type_to_string(hn_object_get_type(object)),
               hn_object_get_id(object));
}

static void object_set_shortcuts(hn_client *client, hn_object *object, const char *shortcuts){

    printf("- CLIENT (%d) SET %s (%d) SHORTCUTS TO \"%s\"\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object),
           shortcuts);
}

static void object_set_enabled(hn_client *client, hn_object *object, hn_bool enabled)
{
    printf("- CLIENT (%d) SET %s (%d) ENABLED TO %s\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object),
           bool_to_string(enabled));
}

static void object_set_checked(hn_client *client, hn_object *object, hn_bool checked)
{
    printf("- CLIENT (%d) SET %s (%d) CHECKED TO %s\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object),
           bool_to_string(checked));
}

static void object_set_active(hn_client *client, hn_object *object)
{
    printf("- CLIENT (%d) SET %s (%d) ACTIVE\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object));
}

static void object_destroy(hn_client *client, hn_object *object)
{
    printf("- CLIENT (%d) DESTROYED %s (%d)\n",
           hn_client_get_id(client),
           object_type_to_string(hn_object_get_type(object)),
           hn_object_get_id(object));
}

/* COMPOSITOR REQUESTS */

void compositor_connected(hn_compositor *compositor)
{
    HN_UNUSED(compositor);
    printf("- COMPOSITOR CONNECTED\n");
}

void compositor_set_active_client(hn_compositor *compositor, hn_client *client, hn_client_pid pid)
{
    HN_UNUSED(compositor);

    if(client)
    {
        printf("- COMPOSITOR SETS CLIENT (%d) ACTIVE\n", hn_client_get_id(client));
        char *msg = "Hey! The compositor says you are the active client!";
        hn_server_send_custom_event_to_client(client, msg, strlen(msg) + 1);
    }
    else
    {
        if(pid == 0)
            printf("- COMPOSITOR SETS NO ACTIVE CLIENT\n");
        else
            printf("- CLIENT SENDS PID (%d) BUT THAT CLIENT IS NOT CONNECTED TO THE GLOBAL MENU\n", pid);
    }

    if(compositor)
    {
        char *msg = "Active client changed!";
        hn_server_send_custom_event_to_compositor(compositor, msg, strlen(msg) + 1);
    }
}

void compositor_send_custom_request(hn_compositor *compositor, void *data, u_int32_t size)
{
    char *msg = data;
    msg[size] = '\0';
    printf("- COMPOSITOR SENT A CUSTOM MESSAGE = \"%s\"\n", msg);

    char *reply = "Hello lovely compositor! <3";
    hn_server_send_custom_event_to_compositor(compositor, reply, strlen(reply) + 1);
}

void compositor_disconnected(hn_compositor *compositor)
{
    HN_UNUSED(compositor);
    printf("- COMPOSITOR DISCONNECTED\n");
}

static hn_server_requests_interface requests_interface =
{
    &client_connected,
    &client_set_app_name,
    &client_send_custom_request,
    &client_disconnected,

    &object_create,
    &object_set_parent,
    &object_set_label,
    &object_set_icon,
    &object_set_shortcuts,
    &object_set_enabled,
    &object_set_checked,
    &object_set_active,
    &object_destroy,

    &compositor_connected,
    &compositor_set_active_client,
    &compositor_send_custom_request,
    &compositor_disconnected

};


int main()
{

    hn_server *server = hn_server_create(NULL, NULL, &requests_interface);

    if(!server)
    {
        printf("Error: Could not create server.\n");
        exit(EXIT_FAILURE);
    }

    printf("\nServer started.\n\n");

    struct pollfd fd;
    fd.fd = hn_server_get_fd(server);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        hn_server_dispatch_requests(server, 0);
    }

    return 0;
}
