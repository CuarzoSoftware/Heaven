#include "../../lib/Heaven-Client.h"
#include <sys/poll.h>
#include <string.h>

const char *object_type_to_string(hv_object_type object_type)
{
    switch (object_type)
    {
        case HV_OBJECT_TYPE_TOP_BAR:
            return "TOP BAR";
            break;
        case HV_OBJECT_TYPE_MENU:
            return "MENU";
            break;
        case HV_OBJECT_TYPE_ACTION:
            return "ACTION";
            break;
        case HV_OBJECT_TYPE_SEPARATOR:
            return "SEPARATOR";
            break;
    }

    return NULL;
}

static void disconnected_from_server(hv_client *client)
{
    HV_UNUSED(client);
    printf("- DISCONNECTED FROM SERVER\n");
}

static void object_destroy(hv_object *object)
{
    hv_object_type type = hv_object_get_type(object);
    printf("- %s (%d) DESTROYED\n", object_type_to_string(type), hv_object_get_id(object));
}

static void server_action_invoke(hv_action *action)
{
    printf("- SERVER INVOKED ACTION (%d)\n", hv_object_get_id(action));
}

static void server_send_custom_event(hv_client *client, void *data, u_int32_t size)
{
    HV_UNUSED(client);
    char *msg = data;
    msg[size] = '\0';
    printf("- SERVER SENT A CUSTOM MESSAGE = \"%s\"\n", msg);
}

hv_client_events_interface events_interface =
{
    &disconnected_from_server,
    &object_destroy,
    &server_action_invoke,
    &server_send_custom_event
};

int main()
{

    hv_client *client = hv_client_create(NULL, "Example Client", NULL, &events_interface);

    if(!client)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("\nClient with pid (%d) started.\n\n", getpid());

    char *msg = "Hello dear server!";
    hv_client_send_custom_request(client, msg, strlen(msg) + 1);

    hv_top_bar *top_bar = hv_object_create(client, HV_OBJECT_TYPE_TOP_BAR, NULL);

    hv_menu *file_menu = hv_object_create(client, HV_OBJECT_TYPE_MENU, NULL);
    hv_menu_set_title(file_menu , "File");
    hv_menu_add_to_top_bar(file_menu , top_bar, NULL);

    hv_menu *edit_menu = hv_object_create(client, HV_OBJECT_TYPE_MENU, NULL);
    hv_menu_set_title(edit_menu , "Edit");
    hv_menu_add_to_top_bar(edit_menu , top_bar, NULL);

    hv_action *action = hv_object_create(client, HV_OBJECT_TYPE_ACTION, NULL);
    hv_action_set_text(action, "Copy");

    hv_pixel dummy_icon[] = {
        255, 255, 255,
        255, 255, 255,
        255, 255, 255
    };
    hv_action_set_icon(action, dummy_icon, 3, 3);

    hv_action_set_shortcuts(action, "Ctrl+C");
    hv_item_add_to_menu(action, file_menu, NULL);

    hv_action *undo_action = hv_object_create(client, HV_OBJECT_TYPE_ACTION, NULL);
    hv_action_set_text(undo_action, "Undo");
    hv_action_set_shortcuts(undo_action, "Ctrl+Z");
    hv_item_add_to_menu(undo_action, file_menu, NULL);

    action = hv_object_create(client, HV_OBJECT_TYPE_ACTION, NULL);
    hv_action_set_text(action, "Redo");
    hv_item_add_to_menu(action, file_menu, NULL);

    action = hv_object_create(client, HV_OBJECT_TYPE_ACTION, NULL);
    hv_action_set_text(action, "Paste from Clipboard History");
    hv_item_add_to_menu(action, file_menu, NULL);


    struct pollfd fd;
    fd.fd = hv_client_get_fd(client);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        if(hv_client_dispatch_events(client, 0) == HV_CONNECTION_LOST)
            break;
    }

    return 0;
}
