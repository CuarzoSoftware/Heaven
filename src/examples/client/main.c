#include "../../lib/client/Heaven-Client.h"

void disconnected_from_server(hv_client *client)
{

}

hv_client_events_interface events_interface =
{
    &disconnected_from_server
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

    hv_top_bar *top_bar = hv_object_create(client, HV_OBJECT_TYPE_TOP_BAR, NULL);

    hv_menu *edit_menu = hv_object_create(client, HV_OBJECT_TYPE_MENU, NULL);
    hv_menu_set_title(edit_menu , "Edit");
    hv_menu_add_to_top_bar(edit_menu , top_bar, NULL);

    hv_menu *file_menu = hv_object_create(client, HV_OBJECT_TYPE_MENU, NULL);
    hv_menu_set_title(file_menu , "File");
    hv_menu_add_to_top_bar(file_menu , top_bar, edit_menu);

    hv_object_remove_from_parent(edit_menu);

    hv_separator *separator = hv_object_create(client, HV_OBJECT_TYPE_SEPARATOR, NULL);
    hv_separator_set_text(separator, "Clipboard");

    hv_item_add_to_menu(separator, file_menu, NULL);

    hv_action *action = hv_object_create(client, HV_OBJECT_TYPE_ACTION, NULL);
    hv_action_set_text(action, "Copy");

    hv_menu_add_to_action(file_menu, action);

    hv_pixel icon[] = {0,255,0, 255,255,255, 0,255,0};
    hv_action_set_icon(action, icon, 3, 3);

    hv_action_set_shortcuts(action, "Ctrl+C, Shift+Ctrl+C");
    hv_action_set_state(action, HV_ACTION_STATE_DISABLED);

    sleep(20);

    hv_client_destroy(client);

    return 0;
}
