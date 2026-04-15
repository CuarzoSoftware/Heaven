#include <CZ/Heaven/Bar/HNBar.h>
#include <CZ/Heaven/Bar/HNLog.h>
#include <CZ/Heaven/Bar/HNCompositor.h>
#include <CZ/Core/CZCore.h>

using namespace CZ;
using namespace CZ::Bar;

int main()
{
    setenv("CZ_HEAVEN_BAR_LOG_LEVEL", "6", 0);

    auto core { CZCore::GetOrMake() };
    auto bar { HNBar::GetOrMake() };

    if (!bar) return 1;

    bar->onCompositorChanged.subscribe(bar.get(), [](HNBar *bar)
    {
        if (bar->compositor())
            HNLog(CZInfo, "Compositor set: {}", bar->compositor()->id());
        else
            HNLog(CZInfo, "Compositor unset");
    });

    while (core->dispatch() >= 0) {}

    return 0;
}
