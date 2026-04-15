#ifndef HNWITHICON_H
#define HNWITHICON_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Bar::HNWithIcon
{
public:
    const std::string &icon() const noexcept { return m_icon; }
protected:
    friend class HNClient;
    std::string m_icon;
};

#endif // HNWITHICON_H
