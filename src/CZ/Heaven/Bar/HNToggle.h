#ifndef HNTOGGLE_H
#define HNTOGGLE_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithShortcut.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

/**
 * @brief Clickable item with a checked/unchecked state displayed in the bar.
 *
 * In addition to the title, icon, shortcut and enabled state, a toggle carries
 * a boolean checked state controlled by the owning client.
 */
class CZ::Bar::HNToggle :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithEnabled,
    public HNWithParent
{
public:
    /**
     * @brief Returns the current checked state.
     *
     * @return true if checked, false otherwise.
     */
    bool checked() const noexcept { return m_checked; }

private:
    friend class HNClient;
    HNToggle(UInt32 id) noexcept :
        HNObject(id, Toggle) {}
    bool m_checked { false };
};

#endif // HNTOGGLE_H
