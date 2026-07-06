#ifndef HNWITHSHORTCUT_H
#define HNWITHSHORTCUT_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for client objects that expose a keyboard shortcut.
 */
class CZ::Client::HNWithShortcut
{
public:
    /**
     * @brief Returns the current keyboard shortcut.
     *
     * @return Const reference to the shortcut string (may be empty).
     */
    const std::string &shortcut() const noexcept { return m_shortcut; }

    /**
     * @brief Updates the shortcut and notifies the bar.
     *
     * If the new shortcut equals the current one, nothing is sent. Otherwise the
     * change is queued and delivered to the bar on the next commit().
     *
     * @param shortcut New shortcut string.
     */
    void setShortcut(const std::string &shortcut) noexcept;

    /// Virtual destructor (makes the mixin polymorphic for safe cross-casts).
    virtual ~HNWithShortcut() noexcept = default;

protected:
    friend class HNClient;
    std::string m_shortcut;
};

#endif // HNWITHSHORTCUT_H
