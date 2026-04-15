#include <CZ/Heaven/Compositor/HNCompositor.h>
#include <CZ/Heaven/Compositor/HNLog.h>
#include <CZ/Core/CZBus.h>
#include <cstring>
#include <systemd/sd-bus.h>

using namespace CZ::Compositor;

static std::weak_ptr<HNCompositor> s_compositor;

struct CZ::Compositor::HNIface
{
    static int RegisterClient(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        const char *id { sd_bus_message_get_sender(m) };
        const char *token;
        sd_bus_message_read(m, "s", &token);
        compositor->onClientRegistered.notify(token, id);
        return sd_bus_reply_method_return(m, "");
    }

    static int BarChanged(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };

        const char *name;
        const char *old_owner;
        const char *new_owner;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (r < 0)
            return r;

        if (old_owner[0] == '\0' && new_owner[0] != '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar appeared");
            compositor->m_isBarAvailable = true;
            const auto activeClient { std::move(compositor->m_activeClientId) };
            compositor->setActiveClient(activeClient);
        } else if (old_owner[0] != '\0' && new_owner[0] == '\0')
        {
            compositor->m_isBarAvailable = false;
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar disappeared");
        } else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar owner changed");
            const auto activeClient { std::move(compositor->m_activeClientId) };
            compositor->setActiveClient(activeClient);
        }

        return 0;
    }
};

static const sd_bus_vtable VTable[]
{
    SD_BUS_VTABLE_START(0),

    SD_BUS_METHOD(
        "RegisterClient",
        "s",
        "",
        HNIface::RegisterClient,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    SD_BUS_VTABLE_END
};

std::shared_ptr<CZ::Compositor::HNCompositor> CZ::Compositor::HNCompositor::GetOrMake() noexcept
{
    int r;

    if (auto compositor = s_compositor.lock())
        return compositor;

    auto bus { CZBus::GetOrMakeUser() };

    if (!bus)
    {
        HNLog(CZFatal, CZLN, "Failed to create CZBus. Make sure a CZCore instance exists before creating a HNCompositor.");
        return {};
    }

    r = sd_bus_add_object_vtable(
        bus->bus(),
        NULL,
        "/org/cuarzo/HeavenCompositor",
        "org.cuarzo.HeavenCompositor",
        VTable,
        NULL);

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to add object vtable. {}", strerror(-r));
        return {};
    }

    r = sd_bus_request_name(bus->bus(), "org.cuarzo.HeavenCompositor", 0);

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to acquire 'org.cuarzo.HeavenCompositor' name. {}", strerror(-r));
        return {};
    }

    r = sd_bus_add_match(
        bus->bus(),
        NULL,
        "type='signal',"
        "sender='org.freedesktop.DBus',"
        "interface='org.freedesktop.DBus',"
        "member='NameOwnerChanged',"
        "arg0='org.cuarzo.HeavenBar'",
        HNIface::BarChanged,
        NULL
    );

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to add signal match. {}", strerror(-r));
        return {};
    }

    auto compositor { std::shared_ptr<HNCompositor>(new HNCompositor(bus)) };
    s_compositor = compositor;
    compositor->m_isBarAvailable = compositor->checkBarState();
    return compositor;
}

void HNCompositor::setActiveClient(const std::string &dbusId) noexcept
{
    if (dbusId == m_activeClientId) return;
    m_activeClientId = dbusId;
    HNLog(CZDebug, CZLN, "Sending active client {}", dbusId);

    sd_bus_message *reply {};

    sd_bus_call_method(
        m_bus->bus(),
        "org.cuarzo.HeavenBar",
        "/org/cuarzo/HeavenBar",
        "org.cuarzo.HeavenBar",
        "SetActiveClient",
        NULL,
        &reply,
        "s",
        dbusId.c_str());
}

HNCompositor::HNCompositor(std::shared_ptr<CZBus> bus) noexcept : m_bus(bus) {}

bool HNCompositor::checkBarState() const noexcept
{
    sd_bus_message *reply {};

    return sd_bus_call_method(
        m_bus->bus(),
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetNameOwner",
        NULL,
        &reply,
        "s",
        "org.cuarzo.HeavenBar") >= 0;
}
