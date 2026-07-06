#include <CZ/Heaven/Client/HNToggle.h>
#include <CZ/Heaven/Client/HNClient.h>

using namespace CZ;
using namespace CZ::Client;

std::shared_ptr<HNToggle> HNToggle::Make(const std::string &title, const std::string &icon, const std::string &shortcut, bool checked, bool enabled, HNObject *parent) noexcept
{
    auto client { HNClient::Get() };

    if (!client) return {};

    auto id { client->getFreeObjectID() };

    if (id == 0) return {};

    auto obj { std::shared_ptr<HNToggle>(new HNToggle(client, id))};

    obj->setTitle(title);
    obj->setIcon(icon);
    obj->setShortcut(shortcut);
    obj->setChecked(checked);
    obj->setEnabled(enabled);
    obj->setParent(parent);

    return obj;
}

void HNToggle::setChecked(bool checked) noexcept
{
    if (m_checked == checked)
        return;
    m_checked = checked;
    client()->sendToggleChecked(this);
}
