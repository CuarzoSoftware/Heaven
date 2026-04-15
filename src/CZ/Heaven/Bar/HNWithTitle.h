#ifndef HNWITHTITLE_H
#define HNWITHTITLE_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Bar::HNWithTitle
{
public:
    const std::string &title() const noexcept { return m_title; }
protected:
    friend class HNClient;
    std::string m_title;
};

#endif // HNWITHTITLE_H
