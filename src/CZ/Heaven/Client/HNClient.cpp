#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithTitle.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>
#include <CZ/Heaven/Client/HNWithIcon.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>
#include <CZ/Heaven/Client/HNLog.h>
#include <cstring>
#include <systemd/sd-bus.h>

using namespace CZ;
using namespace CZ::Client;

static std::weak_ptr<HNClient> s_client;

static const char *BD { "org.cuarzo.HeavenBar" };
static const char *BP { "/org/cuarzo/HeavenBar" };

static int IgnoreCallback(sd_bus_message *, void *, sd_bus_error *) { return 0; }

struct CZ::Client::HNIface
{
    static int BarChanged(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto bar { s_client.lock() };

        const char *name;
        const char *old_owner;
        const char *new_owner;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (r < 0)
            return r;

        if (old_owner[0] == '\0' && new_owner[0] != '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor appeared");

            if (bar->m_compositor)
                bar->m_compositor->m_id = new_owner;
            else
            {
                bar->m_compositor = std::unique_ptr<HNCompositor>(new HNCompositor(new_owner));
                bar->onCompositorChanged.notify(bar.get());
            }

        }
        else if (old_owner[0] != '\0' && new_owner[0] == '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor disappeared");
            bar->m_compositor.reset();
            bar->onCompositorChanged.notify(bar.get());
        }
        else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor owner changed");

            if (bar->m_compositor)
                bar->m_compositor->m_id = new_owner;
            else
            {
                bar->m_compositor = std::unique_ptr<HNCompositor>(new HNCompositor(new_owner));
                bar->onCompositorChanged.notify(bar.get());
            }
        }

        return 0;
    }

    static int CompositorChanged(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto cli { s_client.lock() };

        const char *name;
        const char *old_owner;
        const char *new_owner;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (r < 0)
            return r;

        if (old_owner[0] == '\0' && new_owner[0] != '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor appeared");
            cli->m_compositorId = new_owner;
            cli->sendPrivateHandle();
        }
        else if (old_owner[0] != '\0' && new_owner[0] == '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor disappeared");
            cli->m_compositorId = "";
        }
        else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor owner changed");
            cli->m_compositorId = new_owner;
            cli->sendPrivateHandle();
        }

        return 0;
    }

    static int ObjectClicked(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto cli { s_client.lock() };

        if (strcmp(sd_bus_message_get_sender(m), cli->m_barId.c_str()) != 0)
            return 0;

        UInt32 *objectId;

        int r = sd_bus_message_read(m, "u", &objectId);

        if (r < 0)
            return r;

        auto it { cli->m_objects.find(*objectId) };
        if (it == cli->m_objects.end()) return 0;

        // TODO: Notify

        return 0;
    }

    static int DestroyObjectACK(sd_bus_message *m, void *, sd_bus_error *)
    {
        UInt32 *objectId;
        int r = sd_bus_message_read(m, "u", &objectId);

        if (r < 0)
            return r;

        auto cli { s_client.lock() };

        cli->m_destroyedIds.erase(*objectId);
        cli->m_freedIds.emplace(*objectId);
        return 0;
    }
};

static const sd_bus_vtable VTable[]
{
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD(
        "ObjectClicked",
        "u",    /* object id */
        "",
        HNIface::ObjectClicked,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

std::shared_ptr<HNClient> HNClient::GetOrMake() noexcept
{
    if (auto client = s_client.lock())
        return client;

    int r;

    auto bus { CZBus::GetOrMakeUser() };

    if (!bus)
    {
        HNLog(CZFatal, CZLN, "Failed to create CZBus. Make sure a CZCore instance exists before creating a HNClient.");
        return {};
    }

    r = sd_bus_add_object_vtable(
        bus->bus(),
        NULL,
        "/org/cuarzo/HeavenClient",
        "org.cuarzo.HeavenClient",
        VTable,
        NULL);

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to add object vtable. {}", strerror(-r));
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
        HNLog(CZFatal, CZLN, "Failed to add signal match for arg0='org.cuarzo.HeavenBar'. {}", strerror(-r));
        return {};
    }

    r = sd_bus_add_match(
        bus->bus(),
        NULL,
        "type='signal',"
        "sender='org.freedesktop.DBus',"
        "interface='org.freedesktop.DBus',"
        "member='NameOwnerChanged',"
        "arg0='org.cuarzo.HeavenCompositor'",
        HNIface::CompositorChanged,
        NULL
    );

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to add signal match for arg0='org.cuarzo.HeavenCompositor'. {}", strerror(-r));
        return {};
    }

    auto cli { std::shared_ptr<HNClient>(new HNClient(bus)) };
    s_client = cli;
    return cli;
}

std::shared_ptr<HNClient> HNClient::Get() noexcept
{
    return s_client.lock();
}

void HNClient::setPrivateHandle(const std::string &handle) noexcept
{
    if (handle == m_privateHandle) return;
    m_privateHandle = handle;
    sendPrivateHandle();
}

void HNClient::sendPrivateHandle() noexcept
{
    if (m_compositorId.empty() || m_privateHandle.empty()) return;

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        "org.cuarzo.HeavenCompositor",
        "/org/cuarzo/HeavenCompositor",
        "org.cuarzo.HeavenCompositor",
        "RegisterClient",
        IgnoreCallback,
        NULL,
        "s",
        m_privateHandle.c_str());
}

void HNClient::addObject(HNObject *object) noexcept
{
    m_objects[object->id()] = object;
}

void HNClient::removeObject(HNObject *object) noexcept
{
    UInt32 id  { object->id() };
    m_objects.erase(id);

    if (m_pendingFirstCommit)
    {
        m_freedIds.emplace(id);
    }
    else
    {
        m_destroyedIds.emplace(id);

        sd_bus_slot *slot { NULL };

        sd_bus_call_method_async(
            m_bus->bus(),
            &slot,
            BD, BP, BD,
            "DestroyObject",
            HNIface::DestroyObjectACK,
            NULL,
            "u",
            id);
    }
}

UInt32 HNClient::getFreeObjectID() noexcept
{
    UInt32 id { 0 };

    if (m_freedIds.empty())
    {
        if (m_nextObjectId == 0)
        {
            HNLog(CZError, CZLN, "Objects ID limit reached");
            return 0;
        }
        else
        {
            id = m_nextObjectId;
            m_nextObjectId++;
        }
    }
    else
    {
        id = *m_freedIds.begin();
        m_freedIds.erase(m_freedIds.begin());
    }

    return id;
}

void HNClient::sendObjectTitle(HNWithTitle *obj) noexcept
{
    if (m_pendingFirstCommit) return;

    auto *o { (HNObject*)obj };

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectTitle",
        IgnoreCallback,
        NULL,
        "us",
        o->id(),
        obj->title().c_str());
}

void HNClient::sendObjectShortcut(HNWithShortcut *obj) noexcept
{
    if (m_pendingFirstCommit) return;

    auto *o { (HNObject*)obj };

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectShortcut",
        IgnoreCallback,
        NULL,
        "us",
        o->id(),
        obj->shortcut().c_str());
}

void HNClient::sendObjectIcon(HNWithIcon *obj) noexcept
{
    if (m_pendingFirstCommit) return;

    auto *o { (HNObject*)obj };

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectIcon",
        IgnoreCallback,
        NULL,
        "us",
        o->id(),
        obj->icon().c_str());
}

void HNClient::sendObjectEnabled(HNWithEnabled *obj) noexcept
{
    if (m_pendingFirstCommit) return;

    auto *o { (HNObject*)obj };

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectEnabled",
        IgnoreCallback,
        NULL,
        "ub",
        o->id(),
        obj->enabled());
}
