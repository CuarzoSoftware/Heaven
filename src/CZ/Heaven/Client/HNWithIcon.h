#ifndef HNWITHICON_H
#define HNWITHICON_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for client objects that expose an icon name.
 */
class CZ::Client::HNWithIcon
{
public:
    /**
     * @brief Returns the current icon name.
     *
     * @return Const reference to the icon name string (may be empty).
     */
    const std::string &icon() const noexcept { return m_icon; }

    /**
     * @brief Updates the icon name and notifies the bar.
     *
     * If the new icon equals the current one, nothing is sent. Otherwise the
     * change is queued and delivered to the bar on the next commit().
     *
     * @param icon New icon name.
     */
    void setIcon(const std::string &icon) noexcept;

    /// Virtual destructor (makes the mixin polymorphic for safe cross-casts).
    virtual ~HNWithIcon() noexcept = default;

protected:
    friend class HNClient;
    std::string m_icon;
};

#endif // HNWITHICON_H
