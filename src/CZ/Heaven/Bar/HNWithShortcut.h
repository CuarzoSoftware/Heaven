#ifndef HNWITHSHORTCUT_H
#define HNWITHSHORTCUT_H

#include <CZ/Heaven/Heaven.h>
#include <string>

class CZ::Bar::HNWithShortcut
{
public:
    const std::string &shortcut() const noexcept { return m_shortcut; }
protected:
    friend class HNClient;
    std::string m_shortcut;
};

#endif // HNWITHSHORTCUT_H
