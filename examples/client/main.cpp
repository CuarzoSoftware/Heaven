/**
 * Heaven Client example.
 *
 * Builds a small menu tree (a topbar with two menus containing actions, a
 * toggle and a divider), advertises it to the bar and reacts to click events.
 *
 * Run a Heaven bar and (optionally) a Heaven compositor alongside this program
 * to see the menu appear and the clicks being delivered.
 */

#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNTopbar.h>
#include <CZ/Heaven/Client/HNMenu.h>
#include <CZ/Heaven/Client/HNAction.h>
#include <CZ/Heaven/Client/HNToggle.h>
#include <CZ/Heaven/Client/HNDivider.h>
#include <CZ/Heaven/Client/HNLog.h>
#include <CZ/Core/CZCore.h>
#include <vector>

using namespace CZ;
using namespace CZ::Client;

int main()
{
    setenv("CZ_HEAVEN_CLIENT_LOG_LEVEL", "6", 0);

    auto core { CZCore::GetOrMake() };
    auto client { HNClient::GetOrMake() };

    if (!client)
        return 1;

    // Keep every object alive for the whole session.
    std::vector<std::shared_ptr<HNObject>> keepAlive;

    auto topbar   { HNTopbar::Make() };
    auto fileMenu { HNMenu::Make("File", "", "", true, topbar.get()) };
    auto editMenu { HNMenu::Make("Edit", "", "", true, topbar.get()) };

    auto openAction  { HNAction::Make("Open",  "document-open",   "Ctrl+O", true, fileMenu.get()) };
    auto divider     { HNDivider::Make("", fileMenu.get()) };
    auto quitAction  { HNAction::Make("Quit",  "application-exit", "Ctrl+Q", true, fileMenu.get()) };

    auto wrapToggle  { HNToggle::Make("Word Wrap", "", "Ctrl+W", false, true, editMenu.get()) };

    keepAlive = { topbar, fileMenu, editMenu, openAction, divider, quitAction, wrapToggle };

    openAction->onClicked.subscribe(openAction.get(), [](HNObject*)
    {
        HNLog(CZInfo, "\"Open\" action clicked");
    });

    quitAction->onClicked.subscribe(quitAction.get(), [](HNObject*)
    {
        HNLog(CZInfo, "\"Quit\" action clicked");
    });

    // The toggle flips its own checked state each time it is clicked.
    wrapToggle->onClicked.subscribe(wrapToggle.get(), [w = wrapToggle.get()](HNObject*)
    {
        w->setChecked(!w->checked());
        HNLog(CZInfo, "\"Word Wrap\" toggled to {}", w->checked());
        HNClient::Get()->commit();
    });

    client->setName("Heaven Text Editor");
    client->setActiveTopbar(topbar.get());

    // Identify ourselves to the compositor so it can mark us as the active
    // client (the real handle would come from the lvr-private-handle protocol).
    client->setPrivateHandle("heaven-demo-client");

    // Publish everything. From now on further changes are sent incrementally.
    client->commit();

    HNLog(CZInfo, "Client running. Menu published.");

    while (core->dispatch() >= 0) {}

    return 0;
}
