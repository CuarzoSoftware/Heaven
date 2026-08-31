#ifndef HNWITHICON_H
#define HNWITHICON_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for objects that expose an icon name.
 */
class CZ::HNBarAPI::HNWithIcon
{
public:
    /**
     * @brief Returns the current icon name of the object.
     *
     * @return Const reference to the icon name string (may be empty).
     */
    const std::string &icon() const noexcept { return m_icon; }

    /**
     * @brief Returns whether the icon should be rendered flat (e.g. as a monochrome template).
     *
     * @return `true` if the icon is flat (default), `false` otherwise.
     */
    bool isFlat() const noexcept { return m_isFlat; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithIcon() noexcept = default;

protected:
    friend class HNClient;
    std::string m_icon;
    bool m_isFlat { true };
};

#endif // HNWITHICON_H
