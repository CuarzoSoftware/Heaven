#ifndef HNTOPBAR_H
#define HNTOPBAR_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <CZ/Heaven/Bar/HNWithChildren.h>

/**
 * @brief Top bar container displayed in the bar.
 *
 * A topbar hosts menus and represents the menu set of a single client window.
 * A client may own several topbars and report which one is currently active
 * (see HNClient::activeTopbar()).
 */
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
