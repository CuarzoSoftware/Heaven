#ifndef HNEVENT_H
#define HNEVENT_H

#include <CZ/Heaven/Bar/HNObject.h>
#include <string>

namespace CZ
{
namespace Bar
{
    struct HNEvent
    {
        enum Type
        {
            ClientNameChanged,
            ClientTopbarChanged,
            ObjectCreated,
            ObjectDestroyed,
            ObjectTitleChanged,
            ObjectParentChanged,
            ObjectInsertedAfter,
            ObjectIconChanged,
            ObjectEnabledChanged,
            ObjectShortcutChanged,
            ToggleCheckedChanged
        };

        HNEvent(Type type) noexcept :
            type(type) {}
        virtual ~HNEvent() = default;
        Type type;
    };

    struct HNClientNameChangedEvent : public HNEvent
    {
        HNClientNameChangedEvent(std::string name) noexcept :
            HNEvent(ClientNameChanged),
            name(name) {}
        std::string name;
    };

    struct HNClientTopbarChangedEvent : public HNEvent
    {
        HNClientTopbarChangedEvent(UInt32 topbarId) noexcept :
            HNEvent(ClientTopbarChanged),
            topbarId(topbarId) {}
        UInt32 topbarId;
    };

    struct HNObjectCreatedEvent : public HNEvent
    {
        HNObjectCreatedEvent(UInt32 objectId, HNObject::Type objectType) noexcept :
            HNEvent(ObjectCreated),
            objectId(objectId),
            objectType(objectType) {}
        UInt32 objectId;
        HNObject::Type objectType;
    };

    struct HNObjectDestroyedEvent : public HNEvent
    {
        HNObjectDestroyedEvent(UInt32 objectId) noexcept :
            HNEvent(ObjectDestroyed),
            objectId(objectId) {}
        UInt32 objectId;
    };

    struct HNObjectTitleChangedEvent : public HNEvent
    {
        HNObjectTitleChangedEvent(UInt32 objectId, std::string title) noexcept :
            HNEvent(ObjectTitleChanged),
            objectId(objectId),
            title(title) {}
        UInt32 objectId;
        std::string title;
    };

    struct HNObjectParentChangedEvent : public HNEvent
    {
        HNObjectParentChangedEvent(UInt32 objectId, UInt32 parentId) noexcept :
            HNEvent(ObjectParentChanged),
            objectId(objectId),
            parentId(parentId) {}
        UInt32 objectId;
        UInt32 parentId;
    };

    struct HNObjectInsertedAfterEvent : public HNEvent
    {
        HNObjectInsertedAfterEvent(UInt32 objectId, UInt32 siblingId) noexcept :
            HNEvent(ObjectInsertedAfter),
            objectId(objectId),
            siblingId(siblingId) {}
        UInt32 objectId;
        UInt32 siblingId;
    };

    struct HNObjectIconChangedEvent : public HNEvent
    {
        HNObjectIconChangedEvent(UInt32 objectId, std::string icon) noexcept :
            HNEvent(ObjectIconChanged),
            objectId(objectId),
            icon(icon) {}
        UInt32 objectId;
        std::string icon;
    };

    struct HNObjectEnabledChangedEvent : public HNEvent
    {
        HNObjectEnabledChangedEvent(UInt32 objectId, bool enabled) noexcept :
            HNEvent(ObjectEnabledChanged),
            objectId(objectId),
            enabled(enabled) {}
        UInt32 objectId;
        bool enabled;
    };

    struct HNObjectShortcutChangedEvent : public HNEvent
    {
        HNObjectShortcutChangedEvent(UInt32 objectId, std::string shortcut) noexcept :
            HNEvent(ObjectShortcutChanged),
            objectId(objectId),
            shortcut(shortcut) {}
        UInt32 objectId;
        std::string shortcut;
    };

    struct HNToggleCheckedChangedEvent : public HNEvent
    {
        HNToggleCheckedChangedEvent(UInt32 objectId, bool checked) noexcept :
            HNEvent(ObjectShortcutChanged),
            objectId(objectId),
            checked(checked) {}
        UInt32 objectId;
        bool checked;
    };
}
}

#endif // HNEVENT_H
