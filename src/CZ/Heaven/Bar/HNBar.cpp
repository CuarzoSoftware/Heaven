#include <CZ/Heaven/Bar/HNLog.h>
#include <CZ/Heaven/Bar/HNBar.h>
#include <CZ/Heaven/Bar/HNCompositor.h>
#include <CZ/Heaven/Bar/HNClient.h>
#include <CZ/Heaven/Bar/HNEvent.h>
#include <CZ/Core/CZBus.h>
#include <systemd/sd-bus.h>

using namespace CZ::Bar;

static std::weak_ptr<HNBar> s_bar;

struct CZ::Bar::HNIface
{
    static int ClientDisconnected(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        const char *name;
        const char *old_owner;
        const char *new_owner;

        sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);

        if (old_owner[0] != '\0' && new_owner[0] == '\0')
        {
            auto *client { bar->getClientById(old_owner) };

            if (!client) return 0;

            if (client == bar->m_activeClient)
            {
                bar->m_activeClient = nullptr;
                bar->m_activeClientId = "";
                bar->onActiveClientChanged.notify(bar.get());
            }

            for (auto it = client->m_objects.begin(); it != client->m_objects.end(); it++)
                client->m_events.push(std::make_unique<HNObjectDestroyedEvent>((*it).first));

            client->dispatch();
            bar->onClientDestroyed.notify(client);
            bar->m_clients.erase(old_owner);
        }

        return 0;
    }

    static int CompositorChanged(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        auto bar { s_bar.lock() };

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

    static int SetActiveClient(sd_bus_message *m, void *, sd_bus_error *)
    {
        bool success { false };
        auto bar { s_bar.lock() };
        auto *id { sd_bus_message_get_sender(m) };

        if (bar->compositor() && strcmp(id, bar->compositor()->id().c_str()) == 0)
        {
            success = true;
            const char *id;
            sd_bus_message_read(m, "s", &id);


            if (strcmp(id, "") == 0)
            {
                if (bar->m_activeClient)
                {
                    bar->m_activeClientId = "";
                    bar->m_activeClient = nullptr;
                    bar->onActiveClientChanged.notify(bar.get());
                }
            }
            else
            {
                auto *client { bar->getClientById(id) };

                if (client)
                {
                    if (client != bar->m_activeClient)
                    {
                        bar->m_activeClientId = id;
                        bar->m_activeClient = client;
                        bar->onActiveClientChanged.notify(bar.get());
                    }
                }
                else
                {
                    bar->m_activeClientId = id;
                    HNLog(CZDebug, CZLN, "Client {} set active but still not registered", id);
                }
            }
        }

        return sd_bus_reply_method_return(m, "b", success);
    }

    static int RegisterClient(sd_bus_message *m, void */*userdata*/, sd_bus_error */*ret_error*/)
    {
        bool success { true };
        auto bar { s_bar.lock() };

        if (bar->m_clients.contains(sd_bus_message_get_sender(m)))
        {
            HNLog(CZWarning, CZLN, "Rejected method: Client already registered {}", sd_bus_message_get_sender(m));
            success = false;
        }
        else
        {
            auto client { std::shared_ptr<HNClient>(new HNClient(sd_bus_message_get_sender(m))) };
            bar->m_clients[client->id()] = client;
            HNLog(CZInfo, CZLN, "New client: {}", client->id());
            bar->onClientCreated.notify(client.get());

            if (client->id() == bar->m_activeClientId)
            {
                bar->m_activeClient = client.get();
                bar->onActiveClientChanged.notify(bar.get());
            }
        }

        return sd_bus_reply_method_return(m, "b", success);
    }

    static int SetClientName(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            const char *name;
            sd_bus_message_read(m, "s", &name);
            cli->m_events.push(std::make_unique<HNClientNameChangedEvent>(name));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetClientTopbar(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            sd_bus_message_read(m, "u", &id);
            cli->m_events.push(std::make_unique<HNClientTopbarChangedEvent>(id));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int CreateObject(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id, type;
            sd_bus_message_read(m, "uu", &id, &type);

            if (id > 0 && HNObject::IsValidType(type))
                cli->m_events.push(std::make_unique<HNObjectCreatedEvent>(id, (HNObject::Type)type));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int DestroyObject(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        UInt32 id { 0 };

        if (cli)
        {
            sd_bus_message_read(m, "u", &id);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectDestroyedEvent>(id));
        }

        /* The reply carries the object id back so the client can safely reuse it. */
        return sd_bus_reply_method_return(m, "u", id);
    }

    static int SetObjectTitle(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            const char *title;
            sd_bus_message_read(m, "us", &id, &title);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectTitleChangedEvent>(id, title));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetObjectParent(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id, parentId;
            sd_bus_message_read(m, "uu", &id, &parentId);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectParentChangedEvent>(id, parentId));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int InsertObjectBefore(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id, siblingId;
            sd_bus_message_read(m, "uu", &id, &siblingId);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectInsertedBeforeEvent>(id, siblingId));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetObjectIcon(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            const char *icon;
            sd_bus_message_read(m, "us", &id, &icon);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectIconChangedEvent>(id, icon));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetObjectEnabled(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            int enabled;
            sd_bus_message_read(m, "ub", &id, &enabled);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectEnabledChangedEvent>(id, enabled != 0));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetObjectShortcut(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            const char *shortcut;
            sd_bus_message_read(m, "us", &id, &shortcut);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNObjectShortcutChangedEvent>(id, shortcut));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int SetToggleChecked(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto *cli { bar->getClientById(sd_bus_message_get_sender(m)) };

        if (cli)
        {
            UInt32 id;
            int checked;
            sd_bus_message_read(m, "ub", &id, &checked);

            if (id > 0)
                cli->m_events.push(std::make_unique<HNToggleCheckedChangedEvent>(id, checked != 0));
        }

        return sd_bus_reply_method_return(m, "");
    }

    static int Commit(sd_bus_message *m, void *, sd_bus_error *)
    {
        auto bar { s_bar.lock() };
        auto cli { bar->m_clients.find(sd_bus_message_get_sender(m)) };

        if (cli != bar->m_clients.end())
            cli->second->dispatch();

        return sd_bus_reply_method_return(m, "");
    }
};

static const sd_bus_vtable VTable[]
{
    SD_BUS_VTABLE_START(0),

    /* Compositor requests */

    SD_BUS_METHOD(
        "SetActiveClient",
        "s",    /* in client id */
        "b",
        HNIface::SetActiveClient,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),

    /* Client Requests */

    SD_BUS_METHOD(
        "RegisterClient",
        "",
        "b",
        HNIface::RegisterClient,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetClientName",
        "s", /* Name */
        "",
        HNIface::SetClientName,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetClientTopbar",
        "u", /* Bar ID */
        "",
        HNIface::SetClientTopbar,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "CreateObject",
        "uu", /* Object ID, Object Type */
        "",
        HNIface::CreateObject,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "DestroyObject",
        "u", /* Object ID */
        "u", /* Acknowledged Object ID */
        HNIface::DestroyObject,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetObjectTitle",
        "us", /* Object ID, Title */
        "",
        HNIface::SetObjectTitle,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetObjectParent",
        "uu", /* Object ID, Parent ID (0 to unset parent) */
        "",
        HNIface::SetObjectParent,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "InsertObjectBefore",
        "uu", /* Object ID, Sibling ID (0 to place back) */
        "",
        HNIface::InsertObjectBefore,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetObjectIcon",
        "us", /* Object ID, Icon Name */
        "",
        HNIface::SetObjectIcon,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetObjectEnabled",
        "ub", /* Object ID, Enabled State */
        "",
        HNIface::SetObjectEnabled,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetObjectShortcut",
        "us", /* Object ID, Shortcut */
        "",
        HNIface::SetObjectShortcut,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "SetToggleChecked",
        "ub", /* Object ID, Checked State */
        "",
        HNIface::SetToggleChecked,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_METHOD(
        "Commit",
        "",
        "",
        HNIface::Commit,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

std::shared_ptr<HNBar> HNBar::GetOrMake() noexcept
{
    int r;

    if (auto bar = s_bar.lock())
        return bar;

    auto bus { CZBus::GetOrMakeUser() };

    if (!bus)
    {
        HNLog(CZFatal, CZLN, "Failed to create CZBus. Make sure a CZCore instance exists before creating a HNBar.");
        return {};
    }

    r = sd_bus_add_object_vtable(
        bus->bus(),
        NULL,
        "/org/cuarzo/HeavenBar",
        "org.cuarzo.HeavenBar",
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
        "member='NameOwnerChanged'",
        &HNIface::ClientDisconnected,
        NULL
    );

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to add match for member='NameOwnerChanged'. {}", strerror(-r));
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

    r = sd_bus_request_name(bus->bus(), "org.cuarzo.HeavenBar", 0);

    if (r < 0)
    {
        HNLog(CZFatal, CZLN, "Failed to acquire 'org.cuarzo.HeavenBar' name. {}", strerror(-r));
        return {};
    }

    auto bar { std::shared_ptr<HNBar>(new HNBar(bus)) };
    s_bar = bar;
    bar->checkCompositor();
    return bar;
}

std::shared_ptr<HNBar> HNBar::Get() noexcept
{
    return s_bar.lock();
}

HNClient *HNBar::getClientById(const char *id) const noexcept
{
    auto it { m_clients.find(id) };
    if (it == m_clients.end()) return {};
    return it->second.get();
}

HNBar::HNBar(std::shared_ptr<CZBus> bus) noexcept : m_bus(bus) {}

void HNBar::checkCompositor() noexcept
{
    sd_bus_message *reply {};

    int r = sd_bus_call_method(
        m_bus->bus(),
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetNameOwner",
        NULL,
        &reply,
        "s",
        "org.cuarzo.HeavenCompositor"
        );

    if (r >= 0)
    {
        const char *owner;
        sd_bus_message_read(reply, "s", &owner);

        if (!m_compositor)
        {
            m_compositor = std::unique_ptr<HNCompositor>(new HNCompositor(owner));
            HNLog(CZInfo, CZLN, "Compositor set: {}", compositor()->id());
            onCompositorChanged.notify(this);
        }
        else
            m_compositor->m_id = owner;
    }
}

static int IgnoreClickReply(sd_bus_message *, void *, sd_bus_error *) { return 0; }

void HNBar::sendObjectClicked(const std::string &clientId, UInt32 objectId) noexcept
{
    sd_bus_slot *slot { NULL };

    sd_bus_call_method_async(
        m_bus->bus(),
        &slot,
        clientId.c_str(),
        "/org/cuarzo/HeavenClient",
        "org.cuarzo.HeavenClient",
        "ObjectClicked",
        IgnoreClickReply,
        NULL,
        "u",
        objectId);
}
