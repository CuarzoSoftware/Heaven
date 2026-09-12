#include <CZ/Heaven/Compositor/HNCompositor.h>
#include <CZ/Heaven/Compositor/HNLog.h>
#include <CZ/Core/CZBus.h>
#include <cstring>
#include <systemd/sd-bus.h>

using namespace CZ::HNCompositorAPI;

static std::weak_ptr<HNCompositor> s_compositor;

struct CZ::HNCompositorAPI::HNIface
{
    static int RegisterClient(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        const char *id { sd_bus_message_get_sender(m) };
        const char *token;
        sd_bus_message_read(m, "s", &token);
        HNLog(CZDebug, CZLN, "DBus <- RegisterClient(token={}) from {}", token, id);
        HNLog(CZDebug, CZLN, "Event onClientRegistered: token={} id={}", token, id);
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
            compositor->m_barId = new_owner;
            const auto activeClient { std::move(compositor->m_activeClientId) };
            compositor->setActiveClient(activeClient, compositor->m_activePid, compositor->m_activeUid, compositor->m_activeGid);
        } else if (old_owner[0] != '\0' && new_owner[0] == '\0')
        {
            compositor->m_isBarAvailable = false;
            compositor->m_barId = "";
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar disappeared");
        } else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar owner changed");
            compositor->m_barId = new_owner;
            const auto activeClient { std::move(compositor->m_activeClientId) };
            compositor->setActiveClient(activeClient, compositor->m_activePid, compositor->m_activeUid, compositor->m_activeGid);
        }

        return 0;
    }

    /* Window-management actions requested by the bar's default app-title menu. Only the bar may
     * issue them. */
    static bool FromBar(HNCompositor *compositor, sd_bus_message *m) noexcept
    {
        return compositor && !compositor->m_barId.empty() &&
               strcmp(sd_bus_message_get_sender(m), compositor->m_barId.c_str()) == 0;
    }

    static int HideActiveClient(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- HideActiveClient from bar");
            compositor->onHideActiveClient.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int HideOtherClients(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- HideOtherClients from bar");
            compositor->onHideOtherClients.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int ShowAllClients(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- ShowAllClients from bar");
            compositor->onShowAllClients.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int ToggleActiveClientMinimized(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- ToggleActiveClientMinimized from bar");
            compositor->onToggleActiveClientMinimized.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int ToggleActiveClientMaximized(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- ToggleActiveClientMaximized from bar");
            compositor->onToggleActiveClientMaximized.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int ToggleActiveClientFullscreen(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- ToggleActiveClientFullscreen from bar");
            compositor->onToggleActiveClientFullscreen.notify();
        }
        return sd_bus_reply_method_return(m, "");
    }

    static int CloseActiveClient(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto compositor { s_compositor.lock() };
        if (FromBar(compositor.get(), m))
        {
            HNLog(CZDebug, CZLN, "DBus <- CloseActiveClient from bar");
            compositor->onCloseActiveClient.notify();
        }
        return sd_bus_reply_method_return(m, "");
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
    SD_BUS_METHOD(
        "HideActiveClient",
        "",
        "",
        HNIface::HideActiveClient,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "HideOtherClients",
        "",
        "",
        HNIface::HideOtherClients,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "ShowAllClients",
        "",
        "",
        HNIface::ShowAllClients,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "ToggleActiveClientMinimized",
        "",
        "",
        HNIface::ToggleActiveClientMinimized,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "ToggleActiveClientMaximized",
        "",
        "",
        HNIface::ToggleActiveClientMaximized,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "ToggleActiveClientFullscreen",
        "",
        "",
        HNIface::ToggleActiveClientFullscreen,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "CloseActiveClient",
        "",
        "",
        HNIface::CloseActiveClient,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    SD_BUS_VTABLE_END
};

std::shared_ptr<CZ::HNCompositorAPI::HNCompositor> CZ::HNCompositorAPI::HNCompositor::GetOrMake() noexcept
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

void HNCompositor::setActiveClient(const std::string &dbusId, UInt32 pid, UInt32 uid, UInt32 gid) noexcept
{
    if (dbusId == m_activeClientId && pid == m_activePid && uid == m_activeUid && gid == m_activeGid)
        return;

    m_activeClientId = dbusId;
    m_activePid = pid;
    m_activeUid = uid;
    m_activeGid = gid;
    HNLog(CZDebug, CZLN, "DBus -> SetActiveClient(id={}, pid={}, uid={}, gid={}) to bar", dbusId, pid, uid, gid);

    sd_bus_message *reply {};

    sd_bus_call_method(
        m_bus->bus(),
        "org.cuarzo.HeavenBar",
        "/org/cuarzo/HeavenBar",
        "org.cuarzo.HeavenBar",
        "SetActiveClient",
        NULL,
        &reply,
        "suuu",
        dbusId.c_str(),
        pid,
        uid,
        gid);
}

HNCompositor::HNCompositor(std::shared_ptr<CZBus> bus) noexcept : m_bus(bus) {}

bool HNCompositor::checkBarState() noexcept
{
    sd_bus_message *reply {};

    const int r = sd_bus_call_method(
        m_bus->bus(),
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetNameOwner",
        NULL,
        &reply,
        "s",
        "org.cuarzo.HeavenBar");

    if (r < 0)
        return false;

    const char *owner;
    if (sd_bus_message_read(reply, "s", &owner) >= 0)
        m_barId = owner;

    return true;
}
