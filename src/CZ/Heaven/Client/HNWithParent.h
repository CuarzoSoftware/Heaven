#ifndef HNWITHPARENT_H
#define HNWITHPARENT_H

#include <CZ/Heaven/Heaven.h>
#include <list>

/**
 * @brief Mixin interface for client objects that can be nested inside a parent.
 *
 * Provides the API to attach an object to a parent implementing HNWithChildren
 * and to reorder it among its siblings.
 */
class CZ::Client::HNWithParent
{
public:
    /**
     * @brief Destructor. Detaches this object from its parent.
     *
     * Declared virtual so the mixin is polymorphic, enabling safe cross-casts
     * (via dynamic_cast) between the base subobjects of a concrete object.
     */
    virtual ~HNWithParent() noexcept;

    /**
     * @brief Returns the parent object.
     * @return The parent, or nullptr if none is set.
     */
    HNObject *parent() const noexcept { return m_parent; }

    /**
     * @brief Sets the parent.
     *
     * Appends this object to the parent's children list. If the same parent
     * is provided, the object is not reinserted at the end.
     * Passing nullptr detaches it from its current parent.
     *
     * @param parent Object implementing HNWithChildren, or nullptr.
     * @return false if parent is non-null and invalid, true otherwise.
     */
    bool setParent(HNObject *parent) noexcept;

    /**
     * @brief Inserts this object before the specified sibling.
     *
     * If @p sibling is nullptr, the object is inserted at the end
     * of the current parent's children list.
     *
     * If @p sibling has no parent, the operation has no effect.
     * If @p sibling belongs to a different parent, this object is
     * reparented to match the sibling's parent.
     *
     * @param sibling Pointer to the sibling object before which this object
     *        will be inserted. May be nullptr.
     *
     * @return true if the insertion succeeds; false if @p sibling is invalid.
     */
    bool insertBefore(HNObject *sibling) noexcept;

protected:
    friend class HNClient;
    friend class HNWithChildren;
    HNObject *m_parent {};
    std::list<HNWithParent*>::iterator m_parentLink;
};

#endif // HNWITHPARENT_H
