#include "../../lib/client/Heaven-Client.h"

void test(const char *msg)
{

}

hv_client_events_interface events_interface =
{
    .test = &test
};

int main()
{

    hv_client *client = hv_client_create(NULL, "Example Client", &events_interface);

    if(!client)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Client created!\n");

    hv_top_bar *top_bars[4];
    top_bars[0] = hv_top_bar_create(client, NULL);
    top_bars[1] = hv_top_bar_create(client, NULL);
    top_bars[2] = hv_top_bar_create(client, NULL);
    top_bars[3] = hv_top_bar_create(client, NULL);


    hv_object_destroy(top_bars[1]);
    hv_object_destroy(top_bars[2]);
    hv_top_bar_create(client, NULL);
    hv_top_bar_create(client, NULL);
    hv_top_bar_create(client, NULL);
    hv_menu *menu = hv_menu_create(client, NULL);
    hv_menu_set_title(menu, "Edit");
    hv_menu_add_to_top_bar(menu, top_bars[3], NULL);
    hv_object_destroy((hv_object *)top_bars[0]);
    hv_action *action = hv_action_create(client, NULL);

    hv_action_set_text(action, "Copy");


    //hv_client_destroy(client);

    sleep(10);

    return 0;
}
