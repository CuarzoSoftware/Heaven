#ifndef HNWITHENABLED_H
#define HNWITHENABLED_H

#include <CZ/Heaven/Heaven.h>

class CZ::Client::HNWithEnabled
{
public:
    bool enabled() const noexcept { return m_enabled; }
    void setEnabled(bool enabled) noexcept;
protected:
    friend class HNClient;
    bool m_enabled { true };
};

#endif // HNWITHENABLED_H
