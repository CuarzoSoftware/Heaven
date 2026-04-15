#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>

using namespace CZ;
using namespace CZ::Client;

void HNWithShortcut::setShortcut(const std::string &shortcut) noexcept
{
    if (m_shortcut == shortcut) return;
    m_shortcut = shortcut;

    auto cli { HNClient::Get() };
    cli->sendObjectShortcut(this);
}
