#ifndef HNTOPBAR_H
#define HNTOPBAR_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>

class CZ::Bar::HNTopbar :
    public HNObject,
    public HNWithChildren
{
private:
    friend class HNClient;
    HNTopbar(UInt32 id) noexcept :
        HNObject(id, Topbar) {}
};

#endif // HNTOPBAR_H
