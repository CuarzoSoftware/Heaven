#ifndef HNMENU_H
#define HNMENU_H

#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithChildren.h>
#include <CZ/Heaven/Client/HNWithTitle.h>
#include <CZ/Heaven/Client/HNWithIcon.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>

/**
 * @brief Menu created by a client.
 *
 * A menu can be nested inside a topbar or another menu and can host child
 * objects (menus, actions, toggles and dividers).
 */
class CZ::Client::HNMenu :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithEnabled,
    public HNWithParent,
    public HNWithChildren
{
public:
    /**
     * @brief Creates a new menu and applies the given initial properties.
     *
     * @param title    Initial title.
     * @param icon      Initial icon name.
     * @param shortcut Initial keyboard shortcut.
     * @param enabled  Initial enabled state.
     * @param parent   Object to attach this menu to (must implement HNWithChildren), or nullptr.
     * @return Shared pointer to the new menu, or nullptr on failure.
     */
    static std::shared_ptr<HNMenu> Make(
        const std::string &title = "",
        const std::string &icon = "",
        const std::string &shortcut = "",
        bool enabled = true,
        HNObject *parent = nullptr) noexcept;

private:
    HNMenu(std::shared_ptr<HNClient> client, UInt32 id) noexcept :
        HNObject(client, id, Type::Menu) {}
};

#endif // HNMENU_H
