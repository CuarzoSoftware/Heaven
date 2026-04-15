#ifndef HNTOGGLE_H
#define HNTOGGLE_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithShortcut.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

class CZ::Bar::HNToggle :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithEnabled,
    public HNWithParent
{
public:
    bool checked() const noexcept { return m_checked; }
private:
    friend class HNClient;
    HNToggle(UInt32 id) noexcept :
        HNObject(id, Toggle) {}
    bool m_checked { false };
};

#endif // HNTOGGLE_H
