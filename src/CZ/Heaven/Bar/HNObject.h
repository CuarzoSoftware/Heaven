#ifndef HNITEM_H
#define HNITEM_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <string>

class CZ::Bar::HNObject : public CZObject
{
public:
    enum Type
    {
        Topbar,
        Menu,
        Action,
        Toggle,
        Divider
    };

    UInt32 id() const noexcept { return m_id; }
    Type type() const noexcept { return m_type; }
    HNClient *client() const noexcept { return m_client; }

    static bool IsValidType(UInt32 type) noexcept
    {
        return type >= Topbar && type <= Divider;
    }

protected:
    HNObject(UInt32 id, Type type) noexcept :
        m_id(id), m_type(type) {}

    UInt32 m_id;
    Type m_type;
    HNClient *m_client;
    HNObject *m_parent {};
    std::list<HNObject*>::iterator m_parentLink;
    std::list<HNObject*> m_children;
    std::string m_title;
    std::string m_icon;
    std::string m_shortcut;
    bool m_enabled { true };
    bool m_checked { true };
};

#endif // HNITEM_H
