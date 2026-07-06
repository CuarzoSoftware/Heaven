#ifndef HNBAR_H
#define HNBAR_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZSignal.h>
#include <memory>

/**
 * @brief Core class representing a bar application.
 *
 * A bar application is responsible for displaying and managing UI bars
 * associated with connected clients. It acts as a bridge between the
 * compositor and multiple clients:
 *
 * - The compositor notifies the bar about which client is currently active.
 * - Each client notifies the bar about which of its own bars is active.
 *
 * This class also exposes a set of signals to observe lifecycle changes,
 * client state updates, and object hierarchy updates.
 */
class CZ::Bar::HNBar : public CZObject
{
public:
    /**
     * @brief Retrieves the singleton bar instance, creating it if necessary.
     *
     * @return Shared pointer to the bar instance.
     */
    static std::shared_ptr<HNBar> GetOrMake() noexcept;

    /**
     * @brief Retrieves the existing bar instance.
     *
     * @return Shared pointer to the bar instance, or nullptr if not yet created.
     */
    static std::shared_ptr<HNBar> Get() noexcept;

    /**
     * @brief Returns the currently bound compositor.
     *
     * @return Pointer to the compositor, or nullptr if not yet bound.
     */
    HNCompositor *compositor() const noexcept { return m_compositor.get(); }

    /**
     * @brief Returns the currently active client.
     *
     * The active client is determined by the compositor.
     *
     * @return Pointer to the active client, or nullptr if no client is active.
     */
    HNClient *activeClient() const noexcept { return m_activeClient; }

    /**
     * @brief Retrieves a client by its DBus identifier.
     *
     * @param id Null-terminated DBus identifier string.
     * @return Pointer to the client if found, otherwise nullptr.
     */
    HNClient *getClientById(const char *id) const noexcept;

    /**
     * @brief Emitted when a compositor connection is established or lost.
     */
    CZSignal<HNBar*> onCompositorChanged;

    /**
     * @brief Emitted when the active client changes.
     *
     * @see activeClient()
     */
    CZSignal<HNBar*> onActiveClientChanged;

    /**
     * @brief Emitted when a new client is connected.
     */
    CZSignal<HNClient*> onClientCreated;

    /**
     * @brief Emitted when a client is disconnected.
     *
     * All objects belonging to the client are destroyed before this signal is emitted.
     */
    CZSignal<HNClient*> onClientDestroyed;

    /**
     * @brief Emitted when a client changes its application name.
     */
    CZSignal<HNClient*> onClientNameChanged;

    /**
     * @brief Emitted when a client's active top bar changes.
     *
     * Typically triggered when one of the client’s windows becomes active.
     *
     * @see HNClient::activeTopbar()
     */
    CZSignal<HNClient*> onClientTopbarChanged;

    /**
     * @brief Emitted when a client creates a new object.
     *
     * The object's role/type remains constant during its lifetime.
     */
    CZSignal<HNObject*> onObjectCreated;

    /**
     * @brief Emitted when a client object is destroyed.
     */
    CZSignal<HNObject*> onObjectDestroyed;

    /**
     * @brief Emitted when a client object’s title changes.
     *
     * Objects inheriting the HNWithTitle interface.
     */
    CZSignal<HNObject*> onObjectTitleChanged;

    /**
     * @brief Emitted when a client object’s parent changes.
     *
     * The object is appended to the end of the new parent’s children list.
     * This signal can also indicate that the object has been detached (no parent).
     *
     * Objects inheriting the HNWithIcon interface.
     */
    CZSignal<HNObject*> onObjectParentChanged;

    /**
     * @brief Emitted after an object is inserted before a sibling.
     *
     * This signal is triggered once @p obj has been inserted into the hierarchy,
     * immediately preceding @p sibling.
     *
     * @param obj
     *        The object being inserted.
     *
     * @param sibling
     *        The sibling before which @p obj is inserted.
     *        - If @p sibling belongs to a different parent, @p obj is reparented to match.
     *        - If @p sibling is nullptr, @p obj is appended to the end of its current parent's children list.
     *        - If @p obj has no parent and @p sibling is nullptr, the operation has no effect.
     */
    CZSignal<HNObject* /*obj*/, HNObject* /*sibling (nullable)*/> onObjectInsertedBefore;

    /**
     * @name Object property change signals
     * @{
     */

    /// Emitted when an object's icon changes (objects inheriting the HNWithIcon interface).
    CZSignal<HNObject*> onObjectIconChanged;

    /// Emitted when an object's enabled state changes (objects inheriting the HNWithEnabled interface)
    CZSignal<HNObject*> onObjectEnabledChanged;

    /// Emitted when an object's shortcut changes (objects inheriting the HNWithShortcut interface).
    CZSignal<HNObject*> onObjectShortcutChanged;

    /// Emitted when a toggle object's checked state changes.
    CZSignal<HNToggle*> onToggleCheckedChanged;

    /** @} */

private:
    friend struct HNIface;
    friend class HNObject;
    HNBar(std::shared_ptr<CZBus> bus) noexcept;
    void checkCompositor() noexcept;

    /**
     * @brief Sends a click notification to a client over D-Bus.
     *
     * @param clientId D-Bus unique name of the target client.
     * @param objectId Identifier of the clicked object.
     */
    void sendObjectClicked(const std::string &clientId, UInt32 objectId) noexcept;
    std::shared_ptr<CZBus> m_bus;
    std::unique_ptr<HNCompositor> m_compositor;
    HNClient *m_activeClient {};
    std::string m_activeClientId;
    std::unordered_map<std::string, std::shared_ptr<HNClient>> m_clients;
};

#endif // HNBAR_H
