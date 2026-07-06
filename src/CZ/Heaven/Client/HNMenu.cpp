#include <CZ/Heaven/Client/HNMenu.h>
#include <CZ/Heaven/Client/HNClient.h>

using namespace CZ;
using namespace CZ::Client;

std::shared_ptr<HNMenu> HNMenu::Make(const std::string &title, const std::string &icon, const std::string &shortcut, bool enabled, HNObject *parent) noexcept
{
    auto client { HNClient::Get() };

    if (!client) return {};

    auto id { client->getFreeObjectID() };

    if (id == 0) return {};

    auto obj { std::shared_ptr<HNMenu>(new HNMenu(client, id))};

    obj->setTitle(title);
    obj->setIcon(icon);
    obj->setShortcut(shortcut);
    obj->setEnabled(enabled);
    obj->setParent(parent);

    return obj;
}
