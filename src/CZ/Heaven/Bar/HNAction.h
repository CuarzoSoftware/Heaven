#ifndef HNACTION_H
#define HNACTION_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithTitle.h>
#include <CZ/Heaven/Bar/HNWithIcon.h>
#include <CZ/Heaven/Bar/HNWithParent.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>
#include <CZ/Heaven/Bar/HNWithEnabled.h>

class CZ::Bar::HNAction :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithParent,
    public HNWithEnabled
{
private:
    friend class HNClient;
    HNAction(UInt32 id) noexcept :
        HNObject(id, Action) {}
};

#endif // HNACTION_H
