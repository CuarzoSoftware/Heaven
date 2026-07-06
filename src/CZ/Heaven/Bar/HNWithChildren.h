#ifndef HNWITHCHILDREN_H
#define HNWITHCHILDREN_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZWeak.h>
#include <list>

/**
 * @brief Mixin interface for objects that can host child objects.
 *
 * Objects inheriting this interface (such as HNTopbar and HNMenu) keep an
 * ordered list of children. The order reflects the order in which items
 * should be displayed in the bar.
 */
class CZ::Bar::HNWithChildren
{
public:
    /**
     * @brief Returns the ordered list of child objects.
     *
     * @return Const reference to the list of children, in display order.
     */
    const std::list<HNObject*> &children() const noexcept { return m_children; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithChildren() noexcept = default;

protected:
    friend class HNClient;
    std::list<HNObject*> m_children;
};

#endif // HNWITHCHILDREN_H
