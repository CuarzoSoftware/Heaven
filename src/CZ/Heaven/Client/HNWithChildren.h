#ifndef HNWITHCHILDREN_H
#define HNWITHCHILDREN_H

#include <CZ/Heaven/Heaven.h>
#include <list>

/**
 * @brief Mixin interface for client objects that can host child objects.
 */
class CZ::Client::HNWithChildren
{
public:
    /**
     * @brief Destructor. Detaches every child from this object.
     *
     * Declared virtual so the mixin is polymorphic, enabling safe cross-casts
     * (via dynamic_cast) between the base subobjects of a concrete object.
     */
    virtual ~HNWithChildren() noexcept;

    /**
     * @brief Returns the ordered list of child objects.
     *
     * @return Const reference to the children list, in insertion order.
     */
    const std::list<HNWithParent*> &children() const noexcept { return m_children; }
protected:
    friend class HNClient;
    friend class HNWithParent;
    std::list<HNWithParent*> m_children;
};

#endif // HNWITHCHILDREN_H
