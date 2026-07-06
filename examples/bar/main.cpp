/**
 * Heaven Bar example.
 *
 * Renders (as text) the menu tree of the currently active client and reacts to
 * its updates. It also exercises click delivery: whenever a "Quit" action shows
 * up, it is clicked once, which is delivered back to the owning client.
 */

#include <CZ/Heaven/Bar/HNBar.h>
#include <CZ/Heaven/Bar/HNClient.h>
#include <CZ/Heaven/Bar/HNCompositor.h>
#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNTopbar.h>
#include <CZ/Heaven/Bar/HNToggle.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>
#include <CZ/Heaven/Bar/HNWithShortcut.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>
#include <CZ/Heaven/Bar/HNLog.h>
#include <CZ/Core/CZCore.h>
#include <CZ/Core/CZTimer.h>
#include <string>

using namespace CZ;
using namespace CZ::Bar;

static const char *TypeName(HNObject::Type t)
{
    switch (t)
    {
    case HNObject::Topbar:  return "Topbar";
    case HNObject::Menu:    return "Menu";
    case HNObject::Action:  return "Action";
    case HNObject::Toggle:  return "Toggle";
    case HNObject::Divider: return "Divider";
    }
    return "?";
}

static void PrintObject(HNObject *obj, int depth)
{
    std::string indent(depth * 2, ' ');
    std::string line { indent + "- " + TypeName(obj->type()) };

    if (auto *t = dynamic_cast<HNWithTitle*>(obj); t && !t->title().empty())
        line += " \"" + t->title() + "\"";

    if (auto *s = dynamic_cast<HNWithShortcut*>(obj); s && !s->shortcut().empty())
        line += " [" + s->shortcut() + "]";

    if (auto *tg = dynamic_cast<HNToggle*>(obj))
        line += tg->checked() ? " (checked)" : " (unchecked)";

    if (auto *e = dynamic_cast<HNWithEnabled*>(obj); e && !e->enabled())
        line += " (disabled)";

    HNLog(CZInfo, "{}", line);

    if (auto *c = dynamic_cast<HNWithChildren*>(obj))
        for (auto *child : c->children())
            PrintObject(child, depth + 1);
}

static void PrintActiveMenu()
{
    auto bar { HNBar::Get() };
    auto *client { bar->activeClient() };

    HNLog(CZInfo, "----------------------------------------");

    if (!client)
    {
        HNLog(CZInfo, "No active client");
        return;
    }

    HNLog(CZInfo, "Active client: {} ({})", client->name().empty() ? "<unnamed>" : client->name(), client->id());

    if (auto *topbar = client->activeTopbar())
        PrintObject(topbar, 0);
    else
        HNLog(CZInfo, "Client has no active topbar");
}

// Clicks the first "Quit" action found under an object (depth-first).
static bool ClickQuit(HNObject *obj)
{
    if (obj->type() == HNObject::Action)
        if (auto *t = dynamic_cast<HNWithTitle*>(obj); t && t->title() == "Quit")
        {
            HNLog(CZInfo, "Bar clicks the \"Quit\" action");
            obj->click();
            return true;
        }

    if (auto *c = dynamic_cast<HNWithChildren*>(obj))
        for (auto *child : c->children())
            if (ClickQuit(child))
                return true;

    return false;
}

int main()
{
    setenv("CZ_HEAVEN_BAR_LOG_LEVEL", "6", 0);

    auto core { CZCore::GetOrMake() };
    auto bar { HNBar::GetOrMake() };

    if (!bar)
        return 1;

    // Coalesces bursts of updates (a commit emits many signals) into a single
    // redraw a moment after the last change.
    static CZTimer redraw([](CZTimer*)
    {
        PrintActiveMenu();

        if (auto *client = HNBar::Get()->activeClient())
            if (auto *topbar = client->activeTopbar())
                ClickQuit(topbar);
    });

    bar->onCompositorChanged.subscribe(bar.get(), [](HNBar *bar)
    {
        if (bar->compositor())
            HNLog(CZInfo, "Compositor set: {}", bar->compositor()->id());
        else
            HNLog(CZInfo, "Compositor unset");
    });

    bar->onClientCreated.subscribe(bar.get(), [](HNClient *c)      { HNLog(CZInfo, "Client created: {}", c->id()); });
    bar->onClientDestroyed.subscribe(bar.get(), [](HNClient *c)    { HNLog(CZInfo, "Client destroyed: {}", c->id()); });

    // Any update coalesces into a single redraw shortly afterwards.
    bar->onActiveClientChanged.subscribe(bar.get(), [](auto&&...)  { redraw.start(200); });
    bar->onClientNameChanged.subscribe(bar.get(), [](auto&&...)    { redraw.start(200); });
    bar->onClientTopbarChanged.subscribe(bar.get(), [](auto&&...)  { redraw.start(200); });
    bar->onObjectParentChanged.subscribe(bar.get(), [](auto&&...)  { redraw.start(200); });
    bar->onObjectInsertedBefore.subscribe(bar.get(), [](auto&&...) { redraw.start(200); });
    bar->onObjectTitleChanged.subscribe(bar.get(), [](auto&&...)   { redraw.start(200); });
    bar->onObjectIconChanged.subscribe(bar.get(), [](auto&&...)    { redraw.start(200); });
    bar->onObjectEnabledChanged.subscribe(bar.get(), [](auto&&...) { redraw.start(200); });
    bar->onObjectShortcutChanged.subscribe(bar.get(), [](auto&&...){ redraw.start(200); });
    bar->onToggleCheckedChanged.subscribe(bar.get(), [](auto&&...) { redraw.start(200); });

    while (core->dispatch() >= 0) {}

    return 0;
}
