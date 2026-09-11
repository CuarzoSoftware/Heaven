#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithTitle.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>
#include <CZ/Heaven/Client/HNWithIcon.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithChildren.h>
#include <CZ/Heaven/Client/HNTopbar.h>
#include <CZ/Heaven/Client/HNToggle.h>
#include <CZ/Heaven/Client/HNLog.h>
#include <cstring>
#include <iterator>
#include <systemd/sd-bus.h>

using namespace CZ;
using namespace CZ::HNClientAPI;

static std::weak_ptr<HNClient> s_client;

// Bar destination, path and interface.
static const char *BD { "org.cuarzo.HeavenBar" };
static const char *BP { "/org/cuarzo/HeavenBar" };

// Compositor destination, path and interface.
static const char *CD { "org.cuarzo.HeavenCompositor" };
static const char *CP { "/org/cuarzo/HeavenCompositor" };

static int IgnoreCallback(sd_bus_message *, void *, sd_bus_error *) { return 0; }

struct CZ::HNClientAPI::HNIface
{
    /* Tracks the presence and identity of the bar process. */
    static int BarChanged(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto cli { s_client.lock() };

        const char *name;
        const char *old_owner;
        const char *new_owner;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (r < 0)
            return r;

        if (new_owner[0] == '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar disappeared");
            cli->m_barId = "";
        }
        else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenBar appeared: {}", new_owner);
            cli->m_barId = new_owner;

            // If the client already published its menu, re-send the whole
            // state so the freshly (re)started bar is brought up to date.
            if (!cli->m_pendingFirstCommit)
                cli->flushAll();
        }

        return 0;
    }

    /* Tracks the presence and identity of the compositor process. */
    static int CompositorChanged(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto cli { s_client.lock() };

        const char *name;
        const char *old_owner;
        const char *new_owner;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (r < 0)
            return r;

        if (new_owner[0] == '\0')
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor disappeared");
            cli->m_compositorId = "";
        }
        else
        {
            HNLog(CZInfo, CZLN, "org.cuarzo.HeavenCompositor appeared: {}", new_owner);
            cli->m_compositorId = new_owner;
            cli->sendPrivateHandle();
        }

        return 0;
    }

    /* Invoked by the bar when the user clicks one of this client's objects. */
    static int ObjectClicked(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto cli { s_client.lock() };

        if (strcmp(sd_bus_message_get_sender(m), cli->m_barId.c_str()) != 0)
            return 0;

        UInt32 objectId;

        int r = sd_bus_message_read(m, "u", &objectId);

        if (r < 0)
            return r;

        HNLog(CZDebug, CZLN, "DBus <- ObjectClicked(id={}) from bar", objectId);

        auto it { cli->m_objects.find(objectId) };

        if (it == cli->m_objects.end())
            return 0;

        HNLog(CZDebug, CZLN, "Event onClicked: id={}", objectId);
        it->second->onClicked.notify(it->second);
        return 0;
    }

    /* Reply callback of an asynchronous DestroyObject call. */
    static int DestroyObjectACK(sd_bus_message *m, void *, sd_bus_error *)
    {
        UInt32 objectId;
        int r = sd_bus_message_read(m, "u", &objectId);

        if (r < 0)
            return r;

        auto cli { s_client.lock() };

        if (!cli)
            return 0;

        cli->m_destroyedIds.erase(objectId);
        cli->m_freedIds.emplace(objectId);
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

    // Detect processes that are already present on the bus.
    sd_bus_message *reply {};
    const char *owner;

    if (sd_bus_call_method(bus->bus(), "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "GetNameOwner", NULL, &reply, "s", BD) >= 0)
    {
        sd_bus_message_read(reply, "s", &owner);
        cli->m_barId = owner;
        HNLog(CZInfo, CZLN, "Bar already present: {}", cli->m_barId);
    }

    reply = nullptr;

    if (sd_bus_call_method(bus->bus(), "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "GetNameOwner", NULL, &reply, "s", CD) >= 0)
    {
        sd_bus_message_read(reply, "s", &owner);
        cli->m_compositorId = owner;
        HNLog(CZInfo, CZLN, "Compositor already present: {}", cli->m_compositorId);
    }

    return cli;
}

std::shared_ptr<HNClient> HNClient::Get() noexcept
{
    return s_client.lock();
}

void HNClient::commit() noexcept
{
    if (m_pendingFirstCommit)
    {
        m_pendingFirstCommit = false;

        if (!m_barId.empty())
            flushAll();
    }
    else if (!m_barId.empty())
        sendCommit();
}

void HNClient::setName(const std::string &name) noexcept
{
    if (name == m_name) return;
    m_name = name;
    sendClientName();
}

void HNClient::setActiveTopbar(HNTopbar *topbar) noexcept
{
    if (!topbar || topbar == m_activeTopbar.get()) return;
    m_activeTopbar.reset(topbar);
    sendClientTopbar();
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

    HNLog(CZDebug, CZLN, "DBus -> RegisterClient(handle={}) to compositor", m_privateHandle);

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        CD, CP, CD,
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
    UInt32 id { object->id() };
    m_objects.erase(id);

    if (canSend())
    {
        m_destroyedIds.emplace(id);

        HNLog(CZDebug, CZLN, "DBus -> DestroyObject(id={})", id);

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
    else
    {
        // The bar is not aware of this object (never committed or bar absent),
        // so the id can be reused right away.
        m_freedIds.emplace(id);
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

void HNClient::sendCreateObject(HNObject *o) noexcept
{
    if (!canSend()) return;

    HNLog(CZDebug, CZLN, "DBus -> CreateObject(id={}, type={})", o->id(), (UInt32)o->type());

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "CreateObject",
        IgnoreCallback,
        NULL,
        "uu",
        o->id(),
        (UInt32)o->type());
}

void HNClient::sendObjectTitle(HNWithTitle *obj) noexcept
{
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectTitle(id={}, title={})", o->id(), obj->title());

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
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectShortcut(id={}, shortcut={})", o->id(), obj->shortcut());

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
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectIcon(id={}, icon={})", o->id(), obj->icon());

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

void HNClient::sendObjectIconFlat(HNWithIcon *obj) noexcept
{
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectIconFlat(id={}, flat={})", o->id(), obj->isFlat());

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectIconFlat",
        IgnoreCallback,
        NULL,
        "ub",
        o->id(),
        (int)obj->isFlat());
}

void HNClient::sendObjectEnabled(HNWithEnabled *obj) noexcept
{
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectEnabled(id={}, enabled={})", o->id(), obj->enabled());

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
        (int)obj->enabled());
}

void HNClient::sendObjectParent(HNWithParent *obj) noexcept
{
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };

    HNLog(CZDebug, CZLN, "DBus -> SetObjectParent(id={}, parent={})", o->id(), obj->parent() ? obj->parent()->id() : 0);

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetObjectParent",
        IgnoreCallback,
        NULL,
        "uu",
        o->id(),
        obj->parent() ? obj->parent()->id() : 0);
}

void HNClient::sendInsertObjectBefore(HNWithParent *obj) noexcept
{
    if (!canSend()) return;

    auto *o { dynamic_cast<HNObject*>(obj) };
    auto *parent { dynamic_cast<HNWithChildren*>(obj->parent()) };

    // The sibling is the object that now follows 'obj' in the parent's list,
    // or 0 (append) if 'obj' is the last child.
    UInt32 siblingId { 0 };

    if (parent)
    {
        auto next { std::next(obj->m_parentLink) };

        if (next != parent->m_children.end())
            siblingId = dynamic_cast<HNObject*>(*next)->id();
    }

    HNLog(CZDebug, CZLN, "DBus -> InsertObjectBefore(id={}, sibling={})", o->id(), siblingId);

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "InsertObjectBefore",
        IgnoreCallback,
        NULL,
        "uu",
        o->id(),
        siblingId);
}

void HNClient::sendToggleChecked(HNToggle *obj) noexcept
{
    if (!canSend()) return;

    HNLog(CZDebug, CZLN, "DBus -> SetToggleChecked(id={}, checked={})", obj->id(), obj->checked());

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetToggleChecked",
        IgnoreCallback,
        NULL,
        "ub",
        obj->id(),
        (int)obj->checked());
}

void HNClient::sendClientName() noexcept
{
    if (!canSend()) return;

    HNLog(CZDebug, CZLN, "DBus -> SetClientName(name={})", m_name);

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetClientName",
        IgnoreCallback,
        NULL,
        "s",
        m_name.c_str());
}

void HNClient::sendClientTopbar() noexcept
{
    if (!canSend() || !m_activeTopbar.get()) return;

    HNLog(CZDebug, CZLN, "DBus -> SetClientTopbar(id={})", m_activeTopbar->id());

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "SetClientTopbar",
        IgnoreCallback,
        NULL,
        "u",
        m_activeTopbar->id());
}

void HNClient::sendCommit() noexcept
{
    if (m_barId.empty()) return;

    HNLog(CZDebug, CZLN, "DBus -> Commit");

    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "Commit",
        IgnoreCallback,
        NULL,
        "");
}

void HNClient::sendObjectProperties(HNObject *obj) noexcept
{
    if (auto *t = dynamic_cast<HNWithTitle*>(obj))    sendObjectTitle(t);
    if (auto *i = dynamic_cast<HNWithIcon*>(obj))     { sendObjectIcon(i); sendObjectIconFlat(i); }
    if (auto *s = dynamic_cast<HNWithShortcut*>(obj)) sendObjectShortcut(s);
    if (auto *e = dynamic_cast<HNWithEnabled*>(obj))  sendObjectEnabled(e);
    if (auto *g = dynamic_cast<HNToggle*>(obj))       sendToggleChecked(g);
}

void HNClient::flushAll() noexcept
{
    if (m_barId.empty()) return;

    HNLog(CZDebug, CZLN, "DBus -> RegisterClient + full state flush to bar");

    // 1. (Re)register with the bar.
    sd_bus_slot *slot { NULL };
    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        BD, BP, BD,
        "RegisterClient",
        IgnoreCallback,
        NULL,
        "");

    // 2. Create every object and re-send its properties.
    for (auto &[id, obj] : m_objects)
    {
        sendCreateObject(obj);
        sendObjectProperties(obj);
    }

    // 3. Rebuild the parent-child hierarchy, preserving children order.
    for (auto &[id, obj] : m_objects)
    {
        if (auto *withChildren = dynamic_cast<HNWithChildren*>(obj))
            for (auto *child : withChildren->children())
                sendObjectParent(child);
    }

    // 4. Client-level state.
    sendClientName();
    sendClientTopbar();

    // 5. Apply everything atomically.
    sendCommit();
}
