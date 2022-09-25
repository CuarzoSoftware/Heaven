#include "../../lib/Heaven-Client.h"
#include <sys/poll.h>
#include <string.h>

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

static void disconnected_from_server(hn_client *client)
{
    HN_UNUSED(client);
    printf("- DISCONNECTED FROM SERVER\n");
}

static void object_destroy(hn_object *object)
{
    hn_object_type type = hn_object_get_type(object);
    printf("- %s (%d) DESTROYED\n", object_type_to_string(type), hn_object_get_id(object));
}

static void server_action_invoke(hn_action *action)
{
    printf("- SERVER INVOKED ACTION (%d)\n", hn_object_get_id(action));
}

static void server_toggle_set_checked(hn_toggle *toggle, hn_bool checked)
{
    printf("- SERVER SET TOGGLE (%d) CHECKED TO %s\n", hn_object_get_id(toggle), bool_to_string(checked));
}

static void server_option_set_active(hn_option *option, hn_select *select)
{
    printf("- SERVER SELECTED OPTION (%d) FROM SELECT (%d)\n", hn_object_get_id(option), hn_object_get_id(select));
}

static void server_send_custom_event(hn_client *client, void *data, u_int32_t size)
{
    HN_UNUSED(client);
    char *msg = data;
    msg[size] = '\0';
    printf("- SERVER SENT A CUSTOM MESSAGE = \"%s\"\n", msg);
}

hn_client_events_interface events_interface =
{
    &disconnected_from_server,
    &object_destroy,
    &server_action_invoke,
    &server_toggle_set_checked,
    &server_option_set_active,
    &server_send_custom_event

};

void L(enum HN_RETURN_CODE code, hn_client *client)
{
    if(code == HN_SUCCESS)
        printf("OK\n");
    else if(code == HN_ERROR)
    {
        printf("ERROR: %s\n", hn_client_get_error_string(client));
        exit(1);
    }
    else
    {
        printf("DISCONNECTED FROM SERVER: %s\n", hn_client_get_error_string(client));
        exit(1);
    }
}

void E(hn_object *obj, const char *msg)
{
    if(!obj)
    {
        printf("%s\n", msg);
        exit(1);
    }
}

int main()
{

    hn_client *client = hn_client_create(NULL, "Example Client", NULL, &events_interface);

    if(!client)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("\nClient with pid (%d) started.\n\n", getpid());

    hn_pixel dummy_icon[] = {
        255, 255, 255,
        255, 255, 255,
        255, 255, 255
    };

    enum HN_RETURN_CODE ret;

    char *msg = "Hello dear server!";
    L(hn_client_send_custom_request(client, msg, strlen(msg) + 1), client);

    hn_top_bar *top_bar = hn_top_bar_create(client, NULL);
    E(top_bar, "Create top_bar failed");


    hn_menu *file_menu = hn_menu_create(client, NULL);
    E(top_bar, "Create file_menu failed\n");
    L(hn_menu_set_label(file_menu , "File"), client);
    ret = hn_menu_add_to_top_bar(file_menu , top_bar, NULL);

    if(ret != HN_SUCCESS)
        hn_debug("Error");


    struct pollfd fd;
    fd.fd = hn_client_get_fd(client);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        if(hn_client_dispatch_events(client, 0) == HN_CONNECTION_LOST)
            break;
    }

    return 0;

}
