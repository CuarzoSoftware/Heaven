#include "../../lib/client/Heaven-Client.h"

void test(const char *msg)
{

}

struct hv_client_events_interface events_interface =
{
    .test = &test
};

int main()
{

    struct hv_client *client = hv_client_create(NULL, "Example Client", &events_interface);

    if(!client)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Client created!\n");

    struct hv_menu_bar *menu_bars[4];
    menu_bars[0] = hv_menu_bar_create(client, NULL);
    menu_bars[1] = hv_menu_bar_create(client, NULL);
    menu_bars[2] = hv_menu_bar_create(client, NULL);
    menu_bars[3] = hv_menu_bar_create(client, NULL);

    hv_menu_bar_destroy(menu_bars[1]);
    hv_menu_bar_destroy(menu_bars[2]);
    hv_menu_bar_create(client, NULL);
    hv_menu_bar_create(client, NULL);
    hv_menu_bar_create(client, NULL);
    struct hv_menu *menu = hv_menu_create(client, NULL);
    hv_menu_set_title(menu, "HOLA MIS AMORES!");


    //hv_client_destroy(client);

    sleep(10);

    return 0;
}
