#ifndef HNWITHPARENT_H
#define HNWITHPARENT_H

#include <CZ/Heaven/Heaven.h>
#include <list>

class CZ::Bar::HNWithParent
{
public:
    HNObject *parent() const noexcept { return m_parent; }
protected:
    friend class HNClient;
    HNObject *m_parent {};
    std::list<HNObject*>::iterator m_parentLink;
};

#endif // HNWITHPARENT_H
