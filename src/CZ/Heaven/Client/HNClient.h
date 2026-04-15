#ifndef HNCLIENT_H
#define HNCLIENT_H

#include <CZ/Core/CZBus.h>
#include <CZ/Heaven/Heaven.h>
#include <memory>
#include <unordered_set>

class CZ::Client::HNClient
{
public:
    static std::shared_ptr<HNClient> GetOrMake() noexcept;
    static std::shared_ptr<HNClient> Get() noexcept;

    void setPrivateHandle(const std::string &handle) noexcept;
    void commit() noexcept;

    const std::string &barId() const noexcept { return m_barId; }
    const std::string &compositorId() const noexcept { return m_compositorId; }
    const std::string &privateHandle() const noexcept { return m_privateHandle; }
    std::shared_ptr<CZBus> bus() const noexcept { return m_bus; }

private:
    friend class HNObject;
    friend class HNWithTitle;
    friend class HNWithShortcut;
    friend class HNWithIcon;
    friend struct HNIface;
    HNClient(std::shared_ptr<CZBus> bus) noexcept : m_bus(bus) {}
    void sendPrivateHandle() noexcept;
    void addObject(HNObject *object) noexcept;
    void removeObject(HNObject *object) noexcept;
    UInt32 getFreeObjectID() noexcept;

    void sendObjectTitle(HNWithTitle *obj) noexcept;
    void sendObjectShortcut(HNWithShortcut *obj) noexcept;
    void sendObjectIcon(HNWithIcon *obj) noexcept;
    void sendObjectEnabled(HNWithEnabled *obj) noexcept;

    std::shared_ptr<CZBus> m_bus;

    bool m_pendingFirstCommit { true };

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
