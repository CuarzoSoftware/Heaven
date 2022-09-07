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

    struct hv_client *client = hv_create_client(NULL, "Example Client", &events_interface);

    if(!client)
    {
        printf("Error: Could not connect to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Client created!\n");

    for(int i = 0; i < 100; i++)
    {
        hv_client_create_menu_bar(client, NULL);
        sleep(1);
    }

    return 0;
}
