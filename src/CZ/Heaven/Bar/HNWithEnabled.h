#ifndef HNWITHENABLED_H
#define HNWITHENABLED_H

#include <CZ/Heaven/Heaven.h>

/**
 * @brief Mixin interface for objects that can be enabled or disabled.
 *
 * A disabled object is typically rendered as grayed-out and does not react
 * to user interaction.
 */
class CZ::Bar::HNWithEnabled
{
public:
    /**
     * @brief Returns whether the object is currently enabled.
     *
     * @return true if the object is enabled, false otherwise.
     */
    bool enabled() const noexcept { return m_enabled; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithEnabled() noexcept = default;

protected:
    friend class HNClient;
    bool m_enabled { true };
};

#endif // HNWITHENABLED_H
