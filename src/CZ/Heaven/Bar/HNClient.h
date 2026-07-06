#ifndef HNCLIENT_H
#define HNCLIENT_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Heaven/Bar/HNEvent.h>
#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZWeak.h>
#include <memory>
#include <string>
#include <queue>
#include <unordered_map>

/**
 * @brief Represents, on the bar side, a client connected over D-Bus.
 *
 * A client owns a set of HNObject instances (its menu tree) and reports which
 * of its topbars is currently active. Instances are created and destroyed by
 * the bar library as clients register and disconnect.
 */
class CZ::Bar::HNClient : public CZObject
{
public:
    /**
     * @brief Returns the D-Bus unique name of the client.
     *
     * @return Const reference to the client's D-Bus id.
     */
    const std::string &id() const noexcept { return m_id; }

    /**
     * @brief Returns the application name reported by the client.
     *
     * @return Const reference to the client name (may be empty).
     */
    const std::string &name() const noexcept { return m_name; }

    /**
     * @brief Returns the topbar the client currently wants displayed.
     *
     * @return Pointer to the active topbar, or nullptr if none is set.
     */
    HNTopbar *activeTopbar() const noexcept { return m_activeTopbar.lock().get(); }

    /**
     * @brief Destructor.
     */
    ~HNClient() noexcept;

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
