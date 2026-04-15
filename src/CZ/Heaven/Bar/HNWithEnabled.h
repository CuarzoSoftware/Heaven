#ifndef HNWITHENABLED_H
#define HNWITHENABLED_H

#include <CZ/Heaven/Heaven.h>

class CZ::Bar::HNWithEnabled
{
public:
    bool enabled() const noexcept { return m_enabled; }
protected:
    friend class HNClient;
    bool m_enabled;
};

#endif // HNWITHENABLED_H
