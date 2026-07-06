#ifndef HNCLIENT_H
#define HNCLIENT_H

#include <CZ/Core/CZBus.h>
#include <CZ/Core/CZWeak.h>
#include <CZ/Heaven/Heaven.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>

/**
 * @brief Client-side entry point of the Heaven library.
 *
 * A single HNClient instance manages the connection to the bar and compositor,
 * owns the object-id allocator, and batches menu changes until commit(). Menu
 * objects (HNTopbar, HNMenu, …) are created through their own Make() factories
 * and reference the client that owns them.
 *
 * The instance is a lazily-created singleton; retrieve it with GetOrMake().
 */
class CZ::Client::HNClient
{
public:
    /**
     * @brief Returns the existing HNClient instance or creates it if doesn't exist.
     *
     * Retrieves the shared singleton instance of HNClient. If the instance
     * does not already exist, a new one is created and initialized.
     *
     * @return Shared pointer to the singleton HNClient instance.
     */
    static std::shared_ptr<HNClient> GetOrMake() noexcept;

    /**
     * @brief Returns the existing HNClient singleton instance.
     *
     * Unlike GetOrMake(), this method does not create a new instance if
     * one has not already been initialized.
     *
     * @return Shared pointer to the existing HNClient instance, or
     *         nullptr if no instance has been created yet.
     */
    static std::shared_ptr<HNClient> Get() noexcept;

    /**
     * @brief Requests the bar process to apply all pending changes.
     *
     * Signals the bar to process all queued updates atomically, including
     * object creation requests, property updates, and state changes.
     *
     * If called before the connection with the bar has been established,
     * the pending state is retained and sent once the connection becomes
     * available.
     */
    void commit() noexcept;

    /**
     * @brief Returns the application name advertised to the bar.
     *
     * @return Const reference to the application name (may be empty).
     */
    const std::string &name() const noexcept { return m_name; }

    /**
     * @brief Sets the application name advertised to the bar.
     *
     * The change is delivered to the bar on the next commit() (or immediately
     * if a connection is already established and the first commit has occurred).
     *
     * @param name New application name.
     */
    void setName(const std::string &name) noexcept;

    /**
     * @brief Returns the topbar currently marked as active.
     *
     * @return Pointer to the active topbar, or nullptr if none is set.
     */
    HNTopbar *activeTopbar() const noexcept { return m_activeTopbar.get(); }

    /**
     * @brief Marks a topbar as the one that should currently be displayed.
     *
     * Clients typically set this to the topbar associated with their active
     * window. The change is delivered to the bar on the next commit().
     *
     * @param topbar Topbar to mark active, or nullptr to leave it unchanged.
     */
    void setActiveTopbar(HNTopbar *topbar) noexcept;

    /**
     * @brief Sets the compositor-assigned private Wayland handle.
     *
     * The handle is provided through the `lvr-private-handle` Wayland protocol.
     *
     * After being set, the handle is forwarded over D-Bus so the compositor can
     * associate the Wayland client instance with its corresponding D-Bus id.
     *
     * This method is expected to be called exactly once during client initialization.
     *
     * @param handle Private handle received from the `lvr-private-handle` protocol.
     */
    void setPrivateHandle(const std::string &handle) noexcept;

    /**
     * @brief Returns the private Wayland handle associated with this client.
     *
     * The handle is assigned by the compositor through the
     * `lvr-private-handle` protocol and is used to associate the
     * Wayland client with its D-Bus identity.
     *
     * @return The private handle, or an empty string if it has not been set.
     */
    const std::string &privateHandle() const noexcept { return m_privateHandle; }

    /**
     * @brief Returns the D-Bus unique name of the connected bar process.
     *
     * @return The bar process D-Bus ID, or an empty string if the
     *         connection has not been established.
     */
    const std::string &barId() const noexcept { return m_barId; }

    /**
     * @brief Returns the D-Bus unique name of the connected compositor process.
     *
     * @return The compositor process D-Bus ID, or an empty string if the
     *         connection has not been established.
     */
    const std::string &compositorId() const noexcept { return m_compositorId; }

    /**
     * @brief Returns the shared Cuarzo Core D-Bus instance.
     *
     * Provides access to the underlying D-Bus connection used for
     * communication with the compositor and bar processes.
     *
     * @return Shared pointer to the active CZBus instance.
     */
    std::shared_ptr<CZBus> bus() const noexcept { return m_bus; }

private:
    friend class HNObject;
    friend class HNWithTitle;
    friend class HNWithShortcut;
    friend class HNWithIcon;
    friend class HNWithEnabled;
    friend class HNWithParent;
    friend class HNWithChildren;
    friend class HNTopbar;
    friend class HNMenu;
    friend class HNAction;
    friend class HNToggle;
    friend class HNDivider;
    friend struct HNIface;

    HNClient(std::shared_ptr<CZBus> bus) noexcept : m_bus(bus) {}
    void sendPrivateHandle() noexcept;
    void addObject(HNObject *object) noexcept;
    void removeObject(HNObject *object) noexcept;
    UInt32 getFreeObjectID() noexcept;

    /// @return true if the client is connected to the bar and has committed at least once.
    bool canSend() const noexcept { return !m_pendingFirstCommit && !m_barId.empty(); }

    void sendCreateObject(HNObject *obj) noexcept;
    void sendObjectTitle(HNWithTitle *obj) noexcept;
    void sendObjectShortcut(HNWithShortcut *obj) noexcept;
    void sendObjectIcon(HNWithIcon *obj) noexcept;
    void sendObjectEnabled(HNWithEnabled *obj) noexcept;
    void sendObjectParent(HNWithParent *obj) noexcept;
    void sendInsertObjectBefore(HNWithParent *obj) noexcept;
    void sendToggleChecked(HNToggle *obj) noexcept;
    void sendClientName() noexcept;
    void sendClientTopbar() noexcept;
    void sendCommit() noexcept;

    // Registers with the bar and (re)sends the entire client state.
    void flushAll() noexcept;

    // Sends every property currently set on an object.
    void sendObjectProperties(HNObject *obj) noexcept;

    std::shared_ptr<CZBus> m_bus;

    // Becomes false after the first commit(); until then nothing is sent.
    bool m_pendingFirstCommit { true };

    // Application name advertised to the bar.
    std::string m_name;

    // Topbar currently marked as active (auto-nulled when destroyed).
    CZWeak<HNTopbar> m_activeTopbar;

    // Bar D-BUS ID
    std::string m_barId;

    // Compositor D-BUS ID
    std::string m_compositorId;

    // lvr-private-handle
    std::string m_privateHandle;

    // Greatest used id + 1
    UInt32 m_nextObjectId { 1 };

    // Destroyed objects without bar ack (can't be reused yet)
    std::unordered_set<UInt32> m_destroyedIds;

    // From destroyed objects + bar ack (can be reused)
    std::unordered_set<UInt32> m_freedIds;

    // Created objects (ID, Object)
    std::unordered_map<UInt32, HNObject*> m_objects;
};

#endif // HNCLIENT_H
