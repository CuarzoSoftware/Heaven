#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNWithTitle.h>

using namespace CZ;
using namespace CZ::Client;

void HNWithTitle::setTitle(const std::string &title) noexcept
{
    if (m_title == title) return;
    m_title = title;

    auto cli { HNClient::Get() };
    cli->sendObjectTitle(this);
}
