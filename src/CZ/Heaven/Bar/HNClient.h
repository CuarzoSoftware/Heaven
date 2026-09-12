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
class CZ::HNBarAPI::HNClient : public CZObject
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
     * @brief Returns the process id of the underlying Wayland client.
     *
     * Optionally provided by the compositor via setActiveClient(). 0 means unset.
     */
    UInt32 pid() const noexcept { return m_pid; }

    /**
     * @brief Returns the user id of the underlying Wayland client.
     *
     * Optionally provided by the compositor via setActiveClient(). 0 means unset.
     */
    UInt32 uid() const noexcept { return m_uid; }

    /**
     * @brief Returns the group id of the underlying Wayland client.
     *
     * Optionally provided by the compositor via setActiveClient(). 0 means unset.
     */
    UInt32 gid() const noexcept { return m_gid; }

    /**
     * @brief Asks the client to show its About window (default app-title menu "About <App>").
     */
    void about() noexcept;

    /**
     * @brief Asks the client to show its Settings/Preferences window (default app-title menu).
     */
    void settings() noexcept;

    /**
     * @brief Asks the client to quit (default app-title menu "Quit <App>").
     */
    void quit() noexcept;

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
    UInt32 m_pid { 0 };
    UInt32 m_uid { 0 };
    UInt32 m_gid { 0 };
    std::weak_ptr<HNTopbar> m_activeTopbar;
    std::unordered_map<UInt32, std::shared_ptr<HNObject>> m_objects;
    std::queue<std::unique_ptr<HNEvent>> m_events;
    bool m_destroyed { false };
};

#endif // HNCLIENT_H
