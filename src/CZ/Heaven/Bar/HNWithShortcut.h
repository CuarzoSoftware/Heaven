#ifndef HNWITHSHORTCUT_H
#define HNWITHSHORTCUT_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for objects that expose a keyboard shortcut.
 */
class CZ::Bar::HNWithShortcut
{
public:
    /**
     * @brief Returns the current keyboard shortcut of the object.
     *
     * @return Const reference to the shortcut string (may be empty).
     */
    const std::string &shortcut() const noexcept { return m_shortcut; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithShortcut() noexcept = default;

protected:
    friend class HNClient;
    std::string m_shortcut;
};

#endif // HNWITHSHORTCUT_H
