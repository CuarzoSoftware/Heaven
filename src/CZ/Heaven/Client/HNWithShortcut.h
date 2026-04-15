#ifndef HNWITHSHORTCUT_H
#define HNWITHSHORTCUT_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Client::HNWithShortcut
{
public:
    const std::string &shortcut() const noexcept { return m_shortcut; }
    void setShortcut(const std::string &shortcut) noexcept;
protected:
    friend class HNClient;
    std::string m_shortcut;
};

#endif // HNWITHSHORTCUT_H
