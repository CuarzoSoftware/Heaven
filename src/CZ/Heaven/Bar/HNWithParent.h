#ifndef HNWITHPARENT_H
#define HNWITHPARENT_H

#include <CZ/Heaven/Heaven.h>
#include <list>

/**
 * @brief Mixin interface for objects that can be nested inside a parent.
 *
 * Objects inheriting this interface can be attached to an object implementing
 * the HNWithChildren interface. The parent-child relationship is maintained by
 * the bar library from the events sent by the client.
 */
class CZ::Bar::HNWithParent
{
public:
    /**
     * @brief Returns the current parent of this object.
     *
     * @return Pointer to the parent object, or nullptr if the object is detached.
     */
    HNObject *parent() const noexcept { return m_parent; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithParent() noexcept = default;

protected:
    friend class HNClient;
    HNObject *m_parent {};
    std::list<HNObject*>::iterator m_parentLink;
};

#endif // HNWITHPARENT_H
