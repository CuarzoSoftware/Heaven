#include "../../lib/Heaven-Server.h"
#include <poll.h>

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

static void client_connected(hv_client *client)
{
    pid_t pid;
    hv_client_get_credentials(client, &pid, NULL, NULL);
    printf("- CLIENT (%d) WITH PID (%d) CONNECTED\n", hv_client_get_id(client), pid);
}

static void client_set_app_name(hv_client *client, const char *app_name)
{
    printf("- CLIENT (%d) SET APP NAME = \"%s\"\n", hv_client_get_id(client), app_name);
}

static void client_send_custom_request(hv_client *client, void *data, u_int32_t size)
{
    char *msg = data;
    msg[size] = '\0';
    printf("- CLIENT (%d) SENT A CUSTOM MESSAGE = \"%s\"\n", hv_client_get_id(client), msg);

    char *reply = "Hello my dear client! <3";
    hv_server_send_custom_event(client, reply, strlen(reply) + 1);
}

static void client_disconnected(hv_client *client)
{
    pid_t pid;
    hv_client_get_credentials(client, &pid, NULL, NULL);
    printf("- CLIENT (%d) WITH PID (%d) DISCONNECTED\n", hv_client_get_id(client), pid);
}

static void object_create(hv_client *client, hv_object *object)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id object_id = hv_object_get_id(object);
    hv_object_type object_type = hv_object_get_type(object);
    printf("- CLIENT (%d) CREATED %s (%d)\n", client_id, object_type_to_string(object_type), object_id);

    /* Send a "fake" action invokation */
    if(object_type == HV_OBJECT_TYPE_ACTION)
        hv_action_invoke(object);
}

static void object_remove_from_parent(hv_client *client, hv_object *object)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id object_id = hv_object_get_id(object);
    hv_object_type object_type = hv_object_get_type(object);
    hv_object *parent = hv_object_get_parent(object);
    hv_object_id parent_id = hv_object_get_id(parent);
    hv_object_type parent_type = hv_object_get_type(parent);

    printf("- CLIENT (%d) REMOVED %s (%d) FROM %s (%d)\n",
           client_id,
           object_type_to_string(object_type),
           object_id,
           object_type_to_string(parent_type),
           parent_id);
}

static void object_destroy(hv_client *client, hv_object *object)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id object_id = hv_object_get_id(object);
    hv_object_type object_type = hv_object_get_type(object);

    printf("- CLIENT (%d) DESTROYED %s (%d)\n", client_id, object_type_to_string(object_type), object_id);
}

static void top_bar_set_active(hv_client *client, hv_top_bar *top_bar)
{
    hv_client_id client_id = hv_client_get_id(client);

    if(top_bar)
    {
        hv_object_id object_id = hv_object_get_id(top_bar);
        printf("- CLIENT (%d) SET TOP BAR (%d) ACTIVE\n", client_id, object_id);
    }
    else
        printf("- CLIENT (%d) HAS NO ACTIVE TOPBARS\n", client_id);
}

static void menu_set_title(hv_client *client, hv_menu *menu, const char *title)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id object_id = hv_object_get_id(menu);

    printf("- CLIENT (%d) SET MENU (%d) TITLE = \"%s\"\n", client_id, object_id, title);
}

static void menu_add_to_top_bar(hv_client *client, hv_menu *menu, hv_top_bar *top_bar, hv_menu *before)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id menu_id = hv_object_get_id(menu);
    hv_object_id top_bar_id = hv_object_get_id(top_bar);

    if(before)
    {
        hv_object_id before_id = hv_object_get_id(before);
        printf("- CLIENT (%d) ADDED MENU (%d) TO TOP BAR (%d) BEFORE MENU (%d)\n", client_id, menu_id, top_bar_id, before_id);
    }
    else
    {
        printf("- CLIENT (%d) ADDED MENU (%d) AT THE END OF TOP BAR (%d)\n", client_id, menu_id, top_bar_id);
    }
}

static void menu_add_to_action(hv_client *client, hv_menu *menu, hv_action *action)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id menu_id = hv_object_get_id(menu);
    hv_object_id action_id = hv_object_get_id(action);

    printf("- CLIENT (%d) ADDED MENU (%d) TO ACTION (%d)\n", client_id, menu_id, action_id);
}

static void action_set_icon(hv_client *client, hv_action *action, const unsigned char *pixels, u_int32_t width, u_int32_t height)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id action_id = hv_object_get_id(action);

    if(pixels)
        printf("- CLIENT (%d) SET ACTION (%d) ICON %dx%d\n", client_id, action_id, width, height);
    else
        printf("- CLIENT (%d) REMOVED ACTION (%d) ICON\n", client_id, action_id);
}

static void action_set_text(hv_client *client, hv_action *action, const char *text)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id action_id = hv_object_get_id(action);

    printf("- CLIENT (%d) SET ACTION (%d) TEXT = \"%s\"\n", client_id, action_id, text);
}

static void action_set_shortcuts(hv_client *client, hv_action *action, const char *shortcuts)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id action_id = hv_object_get_id(action);

    printf("- CLIENT (%d) SET ACTION (%d) SHORTCUTS = \"%s\"\n", client_id, action_id, shortcuts);
}

static void action_set_state(hv_client *client, hv_action *action, hv_action_state state)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id action_id = hv_object_get_id(action);

    printf("- CLIENT (%d) SET ACTION (%d) STATE = ", client_id, action_id);

    if(state == HV_ACTION_STATE_ENABLED)
        printf("ENABLED\n");
    else
        printf("DISABLED\n");
}

static void separator_set_text(hv_client *client, hv_separator *separator, const char *text)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id separator_id = hv_object_get_id(separator);

    printf("- CLIENT (%d) SET SEPARATOR (%d) TEXT = \"%s\"\n", client_id, separator_id, text);
}

static void item_add_to_menu(hv_client *client, hv_item *item, hv_menu *menu, hv_item *before)
{
    hv_client_id client_id = hv_client_get_id(client);
    hv_object_id item_id = hv_object_get_id(item);
    hv_object_type item_type = hv_object_get_type(item);
    hv_object_id menu_id = hv_object_get_id(item);

    if(before)
    {
        hv_object_id before_id = hv_object_get_id(before);
        hv_object_type before_type = hv_object_get_type(before);

        printf("- CLIENT (%d) ADDED %s (%d) TO MENU (%d) AFTER %s (%d)\n", client_id, object_type_to_string(item_type), item_id, menu_id, object_type_to_string(before_type), before_id);
    }
    else
        printf("- CLIENT (%d) ADDED %s (%d) TO MENU (%d)\n", client_id, object_type_to_string(item_type), item_id, menu_id);
}

static hv_server_requests_interface requests_interface =
{
    &client_connected,
    &client_set_app_name,
    &client_send_custom_request,
    &client_disconnected,

    &object_create,
    &object_remove_from_parent,
    &object_destroy,

    &top_bar_set_active,

    &menu_set_title,
    &menu_add_to_top_bar,
    &menu_add_to_action,

    &action_set_icon,
    &action_set_text,
    &action_set_shortcuts,
    &action_set_state,

    &separator_set_text,

    &item_add_to_menu
};

int main()
{

    hv_server *server = hv_server_create(NULL, NULL, &requests_interface);

    if(!server)
    {
        printf("Error: Could not create server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Server started.\n");

    struct pollfd fd;
    fd.fd = hv_server_get_fd(server);
    fd.events = POLLIN | POLLHUP;
    fd.revents = 0;

    while(1)
    {
        poll(&fd, 1, -1);

        hv_server_dispatch_requests(server, 0);
    }

    return 0;
}
