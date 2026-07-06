#ifndef HNITEM_H
#define HNITEM_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <string>

/**
 * @brief Base class for all objects shared by a client and displayed in the bar.
 *
 * Every menu item exposed by a client is represented, on the bar side, by a
 * subclass of HNObject (HNTopbar, HNMenu, HNAction, HNToggle, HNDivider).
 *
 * The concrete role of an object is fixed at creation time and never changes
 * during its lifetime. Depending on its role, an object may additionally
 * implement the HNWith* mixin interfaces (title, icon, parent, children, etc.).
 *
 * Instances are owned and managed by the bar library; they are created and
 * destroyed as the owning client requests it.
 */
class CZ::Bar::HNObject : public CZObject
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
     * @brief Returns the client-assigned unique identifier of this object.
     *
     * The identifier is unique within the scope of the owning client.
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
     * @return Pointer to the owning client.
     */
    HNClient *client() const noexcept { return m_client; }

    /**
     * @brief Notifies the owning client that this object was clicked.
     *
     * The bar application calls this method when the user activates the item.
     * The click is forwarded over D-Bus to the owning client, which will emit
     * its own click notification.
     */
    void click() noexcept;

    /**
     * @brief Checks whether a numeric value is a valid object type.
     *
     * @param type Raw value received over D-Bus.
     * @return true if @p type maps to a valid Type, false otherwise.
     */
    static bool IsValidType(UInt32 type) noexcept
    {
        return type >= Topbar && type <= Divider;
    }

protected:
    friend class HNClient;
    friend class HNBar;

    /**
     * @brief Constructs an object with the given id and role.
     *
     * @param id   Client-assigned object identifier.
     * @param type Immutable object role.
     */
    HNObject(UInt32 id, Type type) noexcept :
        m_id(id), m_type(type) {}

    UInt32 m_id;
    Type m_type;
    HNClient *m_client {};
};

#endif // HNITEM_H
