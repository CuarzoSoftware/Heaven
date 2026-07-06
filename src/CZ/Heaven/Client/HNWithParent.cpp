#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithChildren.h>

using namespace CZ;
using namespace CZ::Client;

/**
 * @brief Checks whether @p possibleParent is @p obj itself or one of its descendants.
 *
 * Used to prevent creating cycles in the object hierarchy.
 */
static bool IsObjectOrSubchildOf(HNWithParent *obj, HNWithChildren *possibleParent) noexcept
{
    if (!obj) return false;

    if (dynamic_cast<HNObject*>(obj) == dynamic_cast<HNObject*>(possibleParent))
        return true;

    return IsObjectOrSubchildOf(dynamic_cast<HNWithParent*>(obj->parent()), possibleParent);
}

CZ::Client::HNWithParent::~HNWithParent() noexcept
{
    setParent(nullptr);
}

bool HNWithParent::setParent(HNObject *parent) noexcept
{
    if (m_parent == parent)
        return true;

    auto *self { dynamic_cast<HNObject*>(this) };

    if (parent)
    {
        auto *withChildren { dynamic_cast<HNWithChildren*>(parent) };

        if (!withChildren)
            return false;

        if (self->type() != HNObject::Menu && parent->type() == HNObject::Topbar)
            return false;

        if (IsObjectOrSubchildOf(this, withChildren))
            return false;
    }

    // Detach from the current parent, if any.
    if (m_parent)
    {
        auto *currentParent { dynamic_cast<HNWithChildren*>(m_parent) };
        currentParent->m_children.erase(m_parentLink);
    }

    m_parent = parent;

    // Attach to the new parent, if any.
    if (parent)
    {
        auto *newParent { dynamic_cast<HNWithChildren*>(parent) };
        newParent->m_children.emplace_back(this);
        m_parentLink = std::prev(newParent->m_children.end());
    }

    self->client()->sendObjectParent(this);
    return true;
}

bool HNWithParent::insertBefore(HNObject *sibling) noexcept
{
    auto *self { dynamic_cast<HNObject*>(this) };

    if (sibling)
    {
        if (sibling == self)
            return false;

        auto *siblingWithParent { dynamic_cast<HNWithParent*>(sibling) };

        if (!siblingWithParent || !siblingWithParent->parent())
            return false;

        if (siblingWithParent->parent()->type() == HNObject::Topbar && self->type() != HNObject::Menu)
            return false;

        if (m_parent)
        {
            auto *prevParent { dynamic_cast<HNWithChildren*>(m_parent) };
            prevParent->m_children.erase(m_parentLink);
        }

        auto *newParent { dynamic_cast<HNWithChildren*>(siblingWithParent->parent()) };
        m_parent = siblingWithParent->parent();
        m_parentLink = newParent->m_children.insert(siblingWithParent->m_parentLink, this);
    }
    else
    {
        if (!m_parent)
            return true;

        auto *prevParent { dynamic_cast<HNWithChildren*>(m_parent) };
        prevParent->m_children.erase(m_parentLink);
        prevParent->m_children.emplace_back(this);
        m_parentLink = std::prev(prevParent->m_children.end());
    }

    self->client()->sendInsertObjectBefore(this);
    return true;
}
