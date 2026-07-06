#ifndef HNOBJECT_H
#define HNOBJECT_H

#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Heaven/Heaven.h>
#include <memory>

/**
 * @brief Base class for all menu objects created by a client.
 *
 * A client builds its menu tree out of HNObject subclasses (HNTopbar, HNMenu,
 * HNAction, HNToggle, HNDivider). Objects are reference-counted through
 * std::shared_ptr; destroying the last reference removes the object and, once
 * connected, notifies the bar.
 *
 * The role/type of an object is fixed at construction time.
 */
class CZ::Client::HNObject : public CZObject
{
public:
    /**
     * @brief Enumeration of the possible object roles.
     */
    enum Type
    {
        Topbar, ///< Top bar container that can host menus.
        Menu,   ///< Menu that can host other objects and be nested.
        Action, ///< Clickable action item.
        Toggle, ///< Clickable item with a checked/unchecked state.
        Divider ///< Non-interactive separator.
    };

    /**
     * @brief Returns the unique identifier of this object within the client.
     *
     * @return The object identifier (always greater than 0).
     */
    UInt32 id() const noexcept { return m_id; }

    /**
     * @brief Returns the immutable role/type of this object.
     *
     * @return The object type.
     */
    Type type() const noexcept { return m_type; }

    /**
     * @brief Returns the client that owns this object.
     *
     * @return Shared pointer to the owning client.
     */
    std::shared_ptr<HNClient> client() const noexcept { return m_client; }

    /**
     * @brief Emitted when the bar notifies that this object was clicked.
     *
     * @param object Pointer to the clicked object (this).
     */
    CZSignal<HNObject*> onClicked;

    /**
     * @brief Destructor. Notifies the bar that the object was destroyed.
     */
    ~HNObject() noexcept;

protected:
    /**
     * @brief Constructs an object owned by the given client.
     *
     * @param client Owning client.
     * @param id     Unique object identifier within the client.
     * @param type   Immutable object role.
     */
    HNObject(std::shared_ptr<HNClient> client, UInt32 id, Type type) noexcept;

private:
    std::shared_ptr<HNClient> m_client;
    UInt32 m_id;
    Type m_type;
};

#endif // HNOBJECT_H
