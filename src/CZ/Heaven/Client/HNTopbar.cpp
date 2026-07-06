#include <CZ/Heaven/Client/HNTopbar.h>
#include <CZ/Heaven/Client/HNClient.h>

using namespace CZ;
using namespace CZ::Client;

std::shared_ptr<HNTopbar> HNTopbar::Make() noexcept
{
    auto client { HNClient::Get() };

    if (!client) return {};

    auto id { client->getFreeObjectID() };

    if (id == 0) return {};

    auto obj { std::shared_ptr<HNTopbar>(new HNTopbar(client, id))};

    return obj;
}
