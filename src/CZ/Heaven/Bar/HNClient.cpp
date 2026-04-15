#include <CZ/Heaven/Bar/HNClient.h>
#include <CZ/Heaven/Bar/HNTopbar.h>
#include <CZ/Heaven/Bar/HNBar.h>
#include <CZ/Heaven/Bar/HNMenu.h>
#include <CZ/Heaven/Bar/HNAction.h>
#include <CZ/Heaven/Bar/HNToggle.h>
#include <CZ/Heaven/Bar/HNDivider.h>
#include <CZ/Heaven/Bar/HNEvent.h>
#include <CZ/Heaven/Bar/HNLog.h>

using namespace CZ::Bar;

static bool IsObjectOrSubchildOf(HNObject *obj, HNObject *possibleParent) noexcept
{
    if (!obj) return false;
    if (obj == possibleParent) return true;

    auto *withParent { dynamic_cast<HNWithParent*>(obj) };

    if (!withParent) return false;

    return IsObjectOrSubchildOf(withParent->parent(), possibleParent);
}

void CZ::Bar::HNClient::dispatch() noexcept
{
    auto bar { HNBar::Get() };
    if (!bar) return;

    while (!m_events.empty())
    {
        auto event { std::move(m_events.front()) };
        m_events.pop();

        switch (event->type) {
        case HNEvent::ClientNameChanged:
        {
            auto *e { static_cast<HNClientNameChangedEvent*>(event.get()) };

            if (e->name == m_name)
                continue;

            m_name = e->name;
            bar->onClientNameChanged.notify(this);
            break;
        }
        case HNEvent::ClientTopbarChanged:
        {
            auto *e { static_cast<HNClientTopbarChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->topbarId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Attempt to activate non-existent topbar");
                continue;
            }

            auto topbar { dynamic_pointer_cast<HNTopbar>((*it).second) };

            if (!topbar)
            {
                HNLog(CZDebug, CZLN, "Object is not a topbar");
                continue;
            }

            if (topbar.get() == m_activeTopbar.lock().get())
                continue;

            m_activeTopbar = topbar;
            bar->onClientTopbarChanged.notify(this);
            break;
        }
        case HNEvent::ObjectCreated:
        {
            auto *e { static_cast<HNObjectCreatedEvent*>(event.get()) };

            if (e->objectId == 0)
            {
                HNLog(CZDebug, CZLN, "Invalid object id 0");
                continue;
            }

            if (m_objects.contains(e->objectId))
            {
                HNLog(CZDebug, CZLN, "Object id {} already in use", e->objectId);
                continue;
            }

            std::shared_ptr<HNObject> obj;

            switch (e->objectType) {
            case HNObject::Type::Topbar:
                obj = std::shared_ptr<HNTopbar>(new HNTopbar(e->objectId));
                break;
            case HNObject::Type::Menu:
                obj = std::shared_ptr<HNMenu>(new HNMenu(e->objectId));
                break;
            case HNObject::Type::Action:
                obj = std::shared_ptr<HNAction>(new HNAction(e->objectId));
                break;
            case HNObject::Type::Toggle:
                obj = std::shared_ptr<HNToggle>(new HNToggle(e->objectId));
                break;
            case HNObject::Type::Divider:
                obj = std::shared_ptr<HNDivider>(new HNDivider(e->objectId));
                break;
            default:
                continue;
            }

            m_objects[e->objectId] = obj;
            bar->onObjectCreated.notify(obj.get());
            break;
        }
        case HNEvent::ObjectDestroyed:
        {
            auto *e { static_cast<HNObjectDestroyedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            if (auto topbar = dynamic_cast<HNTopbar*>(it->second.get()))
            {
                if (topbar == m_activeTopbar.lock().get())
                {
                    m_activeTopbar.reset();
                    bar->onClientTopbarChanged.notify(this);
                }
            }

            if (auto withParent = dynamic_cast<HNWithParent*>(it->second.get()))
            {
                auto *parent { (HNWithChildren*)(withParent->m_parent) };

                if (parent)
                {
                    parent->m_children.erase(withParent->m_parentLink);
                    withParent->m_parent = nullptr;
                    bar->onObjectParentChanged.notify((HNObject*)parent);
                }
            }

            if (auto withChildren = dynamic_cast<HNWithChildren*>(it->second.get()))
            {
                while (!withChildren->children().empty())
                {
                    auto *child { (HNWithParent*)withChildren->children().back() };
                    withChildren->m_children.pop_back();
                    child->m_parent = nullptr;
                    bar->onObjectParentChanged.notify((HNObject*)child);
                }
            }

            m_objects.erase(it);
            bar->onObjectDestroyed.notify(it->second.get());
            break;
        }
        case HNEvent::ObjectTitleChanged:
        {
            auto *e { static_cast<HNObjectTitleChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withTitle { dynamic_cast<HNWithTitle*>(it->second.get()) };

            if (!withTitle)
            {
                HNLog(CZDebug, CZLN, "Object {} type has no title", e->objectId);
                continue;
            }

            if (withTitle->title() == e->title)
                continue;

            withTitle->m_title = e->title;
            bar->onObjectTitleChanged.notify(it->second.get());
            break;
        }
        case HNEvent::ObjectParentChanged:
        {
            auto *e { static_cast<HNObjectParentChangedEvent*>(event.get()) };
            auto child { m_objects.find(e->objectId) };

            if (child == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withParent { dynamic_cast<HNWithParent*>(child->second.get()) };

            if (!withParent)
            {
                HNLog(CZDebug, CZLN, "Object {} type cannot have a parent", e->objectId);
                continue;
            }

            if (e->parentId == 0)
            {
                if (!withParent->parent())
                    continue;

                auto *withChildren { (HNWithChildren*)withParent->m_parent };
                withChildren->m_children.erase(withParent->m_parentLink);
                withParent->m_parent = nullptr;
                bar->onObjectParentChanged.notify(child->second.get());
            }
            else
            {
                auto parent { m_objects.find(e->parentId) };

                if (parent == m_objects.end())
                {
                    HNLog(CZDebug, CZLN, "Invalid object id {}", e->parentId);
                    continue;
                }

                if (withParent->parent() == parent->second.get())
                    continue;

                if (IsObjectOrSubchildOf(parent->second.get(), child->second.get()))
                {
                    HNLog(CZDebug, CZLN, "The new parent {} is equal or a subchild of the object {}", e->parentId, e->objectId);
                    continue;
                }

                if (withParent->parent())
                {
                    auto *withChildren { (HNWithChildren*)withParent->m_parent };
                    withChildren->m_children.erase(withParent->m_parentLink);
                    withParent->m_parent = nullptr;
                }

                auto *withChildren { (HNWithChildren*)parent->second.get() };
                withChildren->m_children.emplace_back(child->second.get());
                withParent->m_parent = parent->second.get();
                withParent->m_parentLink = std::prev(withChildren->m_children.end());
                bar->onObjectParentChanged.notify(child->second.get());
            }

            break;
        }
        case HNEvent::ObjectInsertedAfter:
        {
            auto *e { static_cast<HNObjectInsertedAfterEvent*>(event.get()) };

            if (e->objectId == e->siblingId)
            {
                HNLog(CZDebug, CZLN, "Object and sibling are the same");
                continue;
            }

            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withParent { dynamic_cast<HNWithParent*>(it->second.get()) };

            if (!withParent)
            {
                HNLog(CZDebug, CZLN, "Object {} type cannot have a parent", e->objectId);
                continue;
            }

            if (!withParent->parent())
            {
                HNLog(CZDebug, CZLN, "Object {} has no parent", e->objectId);
                continue;
            }

            if (e->siblingId == 0)
            {
                auto *withChildren { (HNWithChildren*)withParent->parent() };

                if (withChildren->children().front() == it->second.get())
                    continue;

                withChildren->m_children.erase(withParent->m_parentLink);
                withChildren->m_children.emplace_front(it->second.get());
                withParent->m_parentLink = withChildren->m_children.begin();
                bar->onObjectInsertedAfter.notify(it->second.get(), nullptr);
            }
            else
            {
                auto sibling { m_objects.find(e->siblingId) };

                if (sibling == m_objects.end())
                {
                    HNLog(CZDebug, CZLN, "Invalid sibling id {}", e->siblingId);
                    continue;
                }

                auto *siblingWithParent { dynamic_cast<HNWithParent*>(sibling->second.get()) };

                if (!siblingWithParent)
                {
                    HNLog(CZDebug, CZLN, "Sibling {} type cannot have a parent", e->siblingId);
                    continue;
                }

                if (!siblingWithParent->parent())
                {
                    HNLog(CZDebug, CZLN, "Sibling {} has no parent", e->siblingId);
                    continue;
                }

                if (withParent->parent() != siblingWithParent->parent())
                {
                    HNLog(CZDebug, CZLN, "Object {} is not sibling of {}", e->objectId, e->siblingId);
                    continue;
                }

                if (std::next(siblingWithParent->m_parentLink) == withParent->m_parentLink)
                    continue;

                auto *withChildren { (HNWithChildren*)withParent->parent() };
                withChildren->m_children.erase(withParent->m_parentLink);
                withParent->m_parentLink = withChildren->m_children.insert(
                    std::next(siblingWithParent->m_parentLink),
                    it->second.get());

                bar->onObjectInsertedAfter.notify(it->second.get(), sibling->second.get());
            }

            break;
        }
        case HNEvent::ObjectIconChanged:
        {
            auto *e { static_cast<HNObjectIconChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withIcon { dynamic_cast<HNWithIcon*>(it->second.get()) };

            if (!withIcon)
            {
                HNLog(CZDebug, CZLN, "Object {} type has no icon", e->objectId);
                continue;
            }

            if (withIcon->icon() == e->icon)
                continue;

            withIcon->m_icon = e->icon;
            bar->onObjectIconChanged.notify(it->second.get());
            break;
        }
        case HNEvent::ObjectEnabledChanged:
        {
            auto *e { static_cast<HNObjectEnabledChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withEnabled { dynamic_cast<HNWithEnabled*>(it->second.get()) };

            if (!withEnabled)
            {
                HNLog(CZDebug, CZLN, "Object {} type is has no 'enabled' param", e->objectId);
                continue;
            }

            if (withEnabled->enabled() == e->enabled)
                continue;

            withEnabled->m_enabled = e->enabled;
            bar->onObjectEnabledChanged.notify(it->second.get());
            break;
        }
        case HNEvent::ObjectShortcutChanged:
        {
            auto *e { static_cast<HNObjectShortcutChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *withShortcut { dynamic_cast<HNWithShortcut*>(it->second.get()) };

            if (!withShortcut)
            {
                HNLog(CZDebug, CZLN, "Object {} type has no shortcut", e->objectId);
                continue;
            }

            if (withShortcut->shortcut() == e->shortcut)
                continue;

            withShortcut->m_shortcut = e->shortcut;
            bar->onObjectShortcutChanged.notify(it->second.get());
            break;
        }
        case HNEvent::ToggleCheckedChanged:
        {
            auto *e { static_cast<HNToggleCheckedChangedEvent*>(event.get()) };
            auto it { m_objects.find(e->objectId) };

            if (it == m_objects.end())
            {
                HNLog(CZDebug, CZLN, "Invalid object id {}", e->objectId);
                continue;
            }

            auto *toggle { dynamic_cast<HNToggle*>(it->second.get()) };

            if (!toggle)
            {
                HNLog(CZDebug, CZLN, "Object {} type is not toggle", e->objectId);
                continue;
            }

            if (toggle->checked() == e->checked)
                continue;

            toggle->m_checked = e->checked;
            bar->onToggleCheckedChanged.notify(toggle);
            break;
        }
        default:
            break;
        }
    }

}
