#ifndef HNWITHENABLED_H
#define HNWITHENABLED_H

#include <CZ/Heaven/Heaven.h>

/**
 * @brief Mixin interface for client objects that can be enabled or disabled.
 */
class CZ::Client::HNWithEnabled
{
public:
    /**
     * @brief Returns whether the object is currently enabled.
     *
     * @return true if enabled, false otherwise.
     */
    bool enabled() const noexcept { return m_enabled; }

    /**
     * @brief Updates the enabled state and notifies the bar.
     *
     * If the new state equals the current one, nothing is sent. Otherwise the
     * change is queued and delivered to the bar on the next commit().
     *
     * @param enabled New enabled state.
     */
    void setEnabled(bool enabled) noexcept;

    /// Virtual destructor (makes the mixin polymorphic for safe cross-casts).
    virtual ~HNWithEnabled() noexcept = default;

protected:
    friend class HNClient;
    bool m_enabled { true };
};

#endif // HNWITHENABLED_H
