#ifndef HNOBJECT_H
#define HNOBJECT_H

#include <CZ/Core/CZObject.h>
#include <CZ/Heaven/Heaven.h>
#include <memory>

class CZ::Client::HNObject : public CZObject
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

protected:
    HNObject(std::shared_ptr<HNClient> client, UInt32 id, Type type) noexcept;
    ~HNObject() noexcept;

private:
    std::shared_ptr<HNClient> m_client;
    UInt32 m_id;
    Type m_type;
};

#endif // HNOBJECT_H
