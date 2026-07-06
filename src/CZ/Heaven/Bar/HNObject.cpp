#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNClient.h>
#include <CZ/Heaven/Bar/HNEvent.h>
#include <CZ/Heaven/Bar/HNBar.h>

using namespace CZ::Bar;

void HNObject::click() noexcept
{
    auto bar { HNBar::Get() };

    if (!bar || !m_client)
        return;

    bar->sendObjectClicked(m_client->id(), m_id);
}
