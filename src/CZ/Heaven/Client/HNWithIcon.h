#ifndef HNWITHICON_H
#define HNWITHICON_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for client objects that expose an icon name.
 */
class CZ::HNClientAPI::HNWithIcon
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

    /**
     * @brief Returns whether the icon should be rendered flat (e.g. as a monochrome template).
     *
     * @return `true` if the icon is flat (default), `false` otherwise.
     */
    bool isFlat() const noexcept { return m_isFlat; }

    /**
     * @brief Sets whether the icon should be rendered flat and notifies the bar.
     *
     * If the new value equals the current one, nothing is sent. Otherwise the
     * change is queued and delivered to the bar on the next commit().
     *
     * @param flat `true` for a flat icon (default), `false` otherwise.
     */
    void setIconFlat(bool flat = true) noexcept;

    /// Virtual destructor (makes the mixin polymorphic for safe cross-casts).
    virtual ~HNWithIcon() noexcept = default;

protected:
    friend class HNClient;
    std::string m_icon;
    bool m_isFlat { true };
};

#endif // HNWITHICON_H
