#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>

using namespace CZ;
using namespace CZ::Client;

void HNWithEnabled::setEnabled(bool enabled) noexcept
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    auto cli { HNClient::Get() };
    cli->sendObjectEnabled(this);
}
