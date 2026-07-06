/**
 * Heaven Compositor example.
 *
 * Stands in for a Wayland compositor: whenever a client authenticates itself
 * (by sending the private handle it received out-of-band), the compositor
 * associates that handle with the client's D-Bus id and marks it as the
 * currently active client, which the bar is then told about.
 */

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

    if (!compositor)
        return 1;

    auto *comp { compositor.get() };

    compositor->onClientRegistered.subscribe(compositor.get(), [comp](const char *handle, const char *dbusId)
    {
        HNLog(CZInfo, "New client, HANDLE: {}, DBUS ID: {}", handle, dbusId);
        comp->setActiveClient(dbusId);
    });

    while (core->dispatch() >= 0) {}

    return 0;
}
