#include "../../lib/server/Heaven-Server.h"
#include <poll.h>

static void client_connected(struct hv_client *client)
{
    printf("+ CLIENT\n");
}

static void client_disconnected(struct hv_client *client)
{
    printf("- CLIENT.\n");
}

static void client_set_app_name_request(struct hv_client *client, const char *app_name)
{
    printf("APP NAME: %s\n", app_name);
}

static void menu_bar_create_request(struct hv_menu_bar *menu_bar)
{
    printf("+ MENU BAR %d\n", hv_object_get_id((struct hv_object*)menu_bar));
}

static void menu_bar_destroy_request(struct hv_menu_bar *menu_bar)
{
    printf("- MENU BAR %d\n", hv_object_get_id((struct hv_object*)menu_bar));
}

static void menu_create_request(struct hv_menu *menu)
{
    printf("+ MENU %d\n", hv_object_get_id((struct hv_object*)menu));
}

static void menu_set_title_request(struct hv_menu *menu, const char *title)
{
    printf("MENU %d TITLE: %s\n", hv_object_get_id((struct hv_object*)menu), title);
}

static void menu_destroy_request(struct hv_menu *menu)
{
    printf("- MENU %d\n", hv_object_get_id((struct hv_object*)menu));
}
static struct hv_server_requests_interface requests_interface =
{
    .client_connected = &client_connected,
    .client_disconnected = &client_disconnected,
    .client_set_app_name_request = &client_set_app_name_request,
    .menu_bar_create_request = &menu_bar_create_request,
    .menu_bar_destroy_request = &menu_bar_destroy_request,
    .menu_create_request = &menu_create_request,
    .menu_set_title_request = &menu_set_title_request,
    .menu_destroy_request = &menu_destroy_request
};

int main()
{

    struct hv_server *server = hv_server_create(NULL, &requests_interface);

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
