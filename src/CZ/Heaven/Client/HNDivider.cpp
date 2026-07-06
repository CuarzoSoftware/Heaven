#include <CZ/Heaven/Client/HNDivider.h>
#include <CZ/Heaven/Client/HNClient.h>

using namespace CZ;
using namespace CZ::Client;

std::shared_ptr<HNDivider> HNDivider::Make(const std::string &title, HNObject *parent) noexcept
{
    auto client { HNClient::Get() };

    if (!client) return {};

    auto id { client->getFreeObjectID() };

    if (id == 0) return {};

    auto obj { std::shared_ptr<HNDivider>(new HNDivider(client, id))};

    obj->setTitle(title);
    obj->setParent(parent);

    return obj;
}
