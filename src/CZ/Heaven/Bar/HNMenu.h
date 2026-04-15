#ifndef HNMENU_H
#define HNMENU_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

class CZ::Bar::HNMenu :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithParent,
    public HNWithChildren,
    public HNWithEnabled
{
public:
private:
    friend class HNClient;
    HNMenu(UInt32 id) noexcept :
        HNObject(id, Menu) {}
};

#endif // HNMENU_H
