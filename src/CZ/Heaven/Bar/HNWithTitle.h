#ifndef HNWITHTITLE_H
#define HNWITHTITLE_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for objects that expose a textual title.
 */
class CZ::Bar::HNWithTitle
{
public:
    /**
     * @brief Returns the current title of the object.
     *
     * @return Const reference to the title string (may be empty).
     */
    const std::string &title() const noexcept { return m_title; }

    /**
     * @brief Virtual destructor.
     *
     * Declared virtual so this mixin is polymorphic, enabling safe
     * cross-casts (via dynamic_cast) between the different base subobjects
     * of a concrete object.
     */
    virtual ~HNWithTitle() noexcept = default;

protected:
    friend class HNClient;
    std::string m_title;
};

#endif // HNWITHTITLE_H
