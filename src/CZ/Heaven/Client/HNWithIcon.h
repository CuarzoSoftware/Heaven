#ifndef HNWITHICON_H
#define HNWITHICON_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Client::HNWithIcon
{
public:
    const std::string &icon() const noexcept { return m_icon; }
    void setIcon(const std::string &icon) noexcept;
protected:
    friend class HNClient;
    std::string m_icon;
};

#endif // HNWITHICON_H
