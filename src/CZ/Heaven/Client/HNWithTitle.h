#ifndef HNWITHTITLE_H
#define HNWITHTITLE_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Client::HNWithTitle
{
public:
    const std::string &title() const noexcept { return m_title; }
    void setTitle(const std::string &title) noexcept;
protected:
    friend class HNClient;
    std::string m_title;
};

#endif // HNWITHTITLE_H
