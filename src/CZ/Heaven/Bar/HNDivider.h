#ifndef HNDIVIDER_H
#define HNDIVIDER_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithParent.h>

class CZ::Bar::HNDivider :
    public HNObject,
    public HNWithTitle,
    public HNWithParent
{
private:
    friend class HNClient;
    HNDivider(UInt32 id) noexcept :
        HNObject(id, Divider) {}
};

#endif // HNDIVIDER_H
