#ifndef HNWITHPARENT_H
#define HNWITHPARENT_H

#include <CZ/Heaven/Heaven.h>

class CZ::Client::HNWithParent
{
public:
    HNWithChildren *parent() const noexcept { return m_parent; }
    void setParent(HNWithChildren *parent) noexcept;
protected:
    friend class HNClient;
    HNWithChildren *m_parent {};
};

#endif // HNWITHPARENT_H
