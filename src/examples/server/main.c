#include "../../lib/server/Heaven-Server.h"
#include <poll.h>

static void client_connected(hv_client *client)
{
    printf("+ CLIENT\n");
}

static void client_set_app_name(hv_client *client, const char *app_name)
{
    printf("* APP NAME = %s\n", app_name);
}

static void client_disconnected(hv_client *client)
{
    printf("- CLIENT.\n");
}

static void object_create(hv_object *object)
{
    printf("+ OBJECT %d\n", hv_object_get_id(object));
}

static void object_remove_from_parent(hv_object *object)
{
    printf("* OBJECT %d PARENT = NULL\n", hv_object_get_id(object));
}

static void object_destroy(hv_object *object)
{
    printf("- OBJECT %d\n", hv_object_get_id(object));
}

static void top_bar_set_active(hv_top_bar *top_bar)
{
    if(top_bar)
        printf("* ACTIVE TOP BAR = %d\n", hv_object_get_id(top_bar));
    else
        printf("- ACTIVE TOP BAR\n");
}

static void menu_set_title(hv_menu *menu, const char *title)
{
    printf("* MENU %d TITLE = %s\n", hv_object_get_id(menu), title);
}

static void menu_add_to_top_bar(hv_menu *menu, hv_top_bar *top_bar, hv_menu *before)
{
    printf("* MENU %d >> TOP BAR %d\n", hv_object_get_id(menu), hv_object_get_id(top_bar));
}

static void menu_add_to_action(hv_menu *menu, hv_action *action)
{
    printf("* MENU %d >> ACTION %d\n", hv_object_get_id(menu), hv_object_get_id(action));
}

static void action_set_icon(hv_action *action, const unsigned char *pixels, UInt32 width, UInt32 height)
{
    printf("* ACTION %d SET ICON\n", hv_object_get_id(action));
}

static void action_set_text(hv_action *action, const char *text)
{
    printf("* ACTION %d TEXT = %s\n", hv_object_get_id(action), text);
}

static void action_set_shortcuts(hv_action *action, const char *shortcuts)
{
    printf("* ACTION %d SHORTUCTS = %s\n", hv_object_get_id(action), shortcuts);
}

static void separator_set_text(hv_separator *action, const char *text)
{
    printf("* SEPARATOR %d TEXT = %s\n", hv_object_get_id(action), text);
}

static void item_add_to_menu(hv_item *item, hv_menu *menu, hv_item *before)
{
    printf("* ITEM %d >> MENU %d\n", hv_object_get_id(item), hv_object_get_id(menu));
}

static hv_server_requests_interface requests_interface =
{
    &client_connected,
    &client_set_app_name,
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

    &separator_set_text,

    &item_add_to_menu

};

int main()
{

    hv_server *server = hv_server_create(NULL, &requests_interface);

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

        hv_server_handle_requests(server, 0);
    }

    return 0;
}
