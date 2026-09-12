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
class CZ::HNCompositorAPI::HNCompositor : public CZObject
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
     * @param pid Optional process id of the underlying Wayland client (0 = unset).
     * @param uid Optional user id of the underlying Wayland client (0 = unset).
     * @param gid Optional group id of the underlying Wayland client (0 = unset).
     */
    void setActiveClient(const std::string &dbusId, UInt32 pid = 0, UInt32 uid = 0, UInt32 gid = 0) noexcept;

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

    /**
     * @brief Emitted when the bar requests hiding the active client's windows ("Hide <App>").
     *
     * The compositor should hide (minimize) the windows of the currently active client.
     */
    CZSignal<> onHideActiveClient;

    /**
     * @brief Emitted when the bar requests hiding every client except the active one ("Hide Others").
     */
    CZSignal<> onHideOtherClients;

    /**
     * @brief Emitted when the bar requests showing (unhiding) every client's windows ("Show All").
     */
    CZSignal<> onShowAllClients;

    /**
     * @brief Emitted when the bar requests toggling the active client's minimized state.
     */
    CZSignal<> onToggleActiveClientMinimized;

    /**
     * @brief Emitted when the bar requests toggling the active client's maximized state.
     */
    CZSignal<> onToggleActiveClientMaximized;

    /**
     * @brief Emitted when the bar requests toggling the active client's fullscreen state.
     */
    CZSignal<> onToggleActiveClientFullscreen;

    /**
     * @brief Emitted when the bar requests closing the active client's window.
     */
    CZSignal<> onCloseActiveClient;

private:
    friend struct HNIface;
    HNCompositor(std::shared_ptr<CZBus> bus) noexcept;
    bool checkBarState() noexcept;
    std::shared_ptr<CZBus> m_bus;
    std::string m_activeClientId;
    std::string m_barId;
    UInt32 m_activePid { 0 };
    UInt32 m_activeUid { 0 };
    UInt32 m_activeGid { 0 };
    bool m_isBarAvailable {};
};

#endif // HNCOMPOSITOR_H
