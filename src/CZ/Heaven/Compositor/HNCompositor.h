#ifndef HNCOMPOSITOR_H
#define HNCOMPOSITOR_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <memory>

/**
 * @brief Core class representing a Wayland compositor integration.
 *
 * This class acts as the bridge between a Wayland compositor and the bar
 * application. It is responsible for:
 *
 * - Tracking Wayland clients and associating them with their DBus identifiers.
 * - Notifying the bar application which client is currently active.
 */
class CZ::Compositor::HNCompositor : public CZObject
{
public:
    /**
     * @brief Retrieves the singleton compositor instance, creating it if necessary.
     *
     * @return Shared pointer to the compositor instance, or nullptr on failure.
     */
    static std::shared_ptr<HNCompositor> GetOrMake() noexcept;

    /**
     * @brief Retrieves the existing compositor instance.
     *
     * @return Shared pointer to the compositor instance, or nullptr if not yet created.
     */
    static std::shared_ptr<HNCompositor> Get() noexcept;

    /**
     * @brief Sets the currently active client.
     *
     * This notifies the bar application which client should be considered active.
     *
     * @param dbusId The DBus identifier of the active client.
     *               Passing an empty string ("") clears the active client.
     */
    void setActiveClient(const std::string &dbusId) noexcept;

    /**
     * @brief Emitted when a Wayland client is registered.
     *
     * This signal provides the association between:
     * - A compositor-specific private handle (sent to the Wayland client via
     *   a the private handle protocol), and
     * - The corresponding DBus identifier used by the bar application.
     *
     * The compositor should store this mapping and later use the DBus identifier
     * when calling setActiveClient().
     *
     * @param privateHandle Opaque identifier used internally by the compositor
     *                      to reference the Wayland client.
     * @param dbusId DBus identifier associated with the client.
     */
    CZSignal<const char* /*privateHandle*/, const char* /*dbusId*/> onClientRegistered;

private:
    friend struct HNIface;
    HNCompositor(std::shared_ptr<CZBus> bus) noexcept;
    bool checkBarState() const noexcept;
    std::shared_ptr<CZBus> m_bus;
    std::string m_activeClientId;
    bool m_isBarAvailable {};
};

#endif // HNCOMPOSITOR_H
