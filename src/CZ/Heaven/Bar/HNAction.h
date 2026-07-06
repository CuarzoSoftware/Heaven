#ifndef HNACTION_H
#define HNACTION_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithShortcut.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

/**
 * @brief Clickable action item displayed in the bar.
 *
 * When activated (see HNObject::click()), the click is forwarded to the owning
 * client. It exposes a title, icon, shortcut and enabled state.
 */
class CZ::Bar::HNAction :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithParent,
    public HNWithEnabled
{
private:
    friend class HNClient;
    HNAction(UInt32 id) noexcept :
        HNObject(id, Action) {}
};

#endif // HNACTION_H
