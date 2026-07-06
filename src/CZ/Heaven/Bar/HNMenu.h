#ifndef HNMENU_H
#define HNMENU_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithShortcut.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

/**
 * @brief Menu displayed in the bar.
 *
 * A menu can be nested inside a topbar or another menu and can host child
 * objects. It exposes a title, icon, shortcut and enabled state.
 */
class CZ::Bar::HNMenu :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithParent,
    public HNWithChildren,
    public HNWithEnabled
{
private:
    friend class HNClient;
    HNMenu(UInt32 id) noexcept :
        HNObject(id, Menu) {}
};

#endif // HNMENU_H
