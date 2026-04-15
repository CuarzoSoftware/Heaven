#ifndef HNWITHCHILDREN_H
#define HNWITHCHILDREN_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZWeak.h>
#include <list>

class CZ::Bar::HNWithChildren
{
public:
    const std::list<HNObject*> &children() const noexcept { return m_children; }
protected:
    friend class HNClient;
    std::list<HNObject*> m_children;
};

#endif // HNWITHCHILDREN_H
