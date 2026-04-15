#include <CZ/Heaven/Compositor/HNLog.h>
#include <CZ/Heaven/Compositor/HNCompositor.h>
#include <CZ/Core/CZCore.h>

using namespace CZ;
using namespace CZ::Compositor;

int main()
{
    setenv("CZ_HEAVEN_COMPOSITOR_LOG_LEVEL", "6", 0);

    auto core { CZCore::GetOrMake() };
    auto compositor { HNCompositor::GetOrMake() };

    if (!compositor) return 1;

    auto *comp { compositor.get() };

    compositor->onClientRegistered.subscribe(compositor.get(), [comp](const char *handle, const char *dbusId){
        HNLog(CZInfo, "New client, HANDLE: {}, DBUS ID: {}", handle, dbusId);
        comp->setActiveClient(dbusId);
    });

    while (core->dispatch() >= 0) {}

    return 0;
}
