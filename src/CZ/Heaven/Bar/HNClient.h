#ifndef HNCLIENT_H
#define HNCLIENT_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <memory>
#include <string>
#include <queue>

class CZ::Bar::HNClient : public CZObject
{
public:
    const std::string &id() const noexcept { return m_id; }
    const std::string &name() const noexcept { return m_name; }
    HNTopbar *activeTopbar() const noexcept { return m_activeTopbar.lock().get(); }
private:
    friend struct HNIface;
    friend class HNBar;
    HNClient(const std::string &id) noexcept :
        m_id(id) {}
    void dispatch() noexcept;
    std::string m_id;
    std::string m_name;
    std::weak_ptr<HNTopbar> m_activeTopbar;
    std::unordered_map<UInt32, std::shared_ptr<HNObject>> m_objects;
    std::queue<std::unique_ptr<HNEvent>> m_events;
    bool m_destroyed { false };
};

#endif // HNCLIENT_H
