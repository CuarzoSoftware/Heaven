#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNWithIcon.h>

using namespace CZ;
using namespace CZ::HNClientAPI;

void HNWithIcon::setIcon(const std::string &icon) noexcept
{
    if (m_icon == icon) return;
    m_icon = icon;

    auto cli { HNClient::Get() };
    cli->sendObjectIcon(this);
}

void HNWithIcon::setIconFlat(bool flat) noexcept
{
    if (m_isFlat == flat) return;
    m_isFlat = flat;

    auto cli { HNClient::Get() };
    cli->sendObjectIconFlat(this);
}
