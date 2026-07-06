#ifndef HNWITHTITLE_H
#define HNWITHTITLE_H

#include <CZ/Heaven/Heaven.h>
#include <string>

/**
 * @brief Mixin interface for client objects that expose a textual title.
 */
class CZ::Client::HNWithTitle
{
public:
    /**
     * @brief Returns the current title.
     *
     * @return Const reference to the title string (may be empty).
     */
    const std::string &title() const noexcept { return m_title; }

    /**
     * @brief Updates the title and notifies the bar.
     *
     * If the new title equals the current one, nothing is sent. Otherwise the
     * change is queued and delivered to the bar on the next commit().
     *
     * @param title New title.
     */
    void setTitle(const std::string &title) noexcept;

    /// Virtual destructor (makes the mixin polymorphic for safe cross-casts).
    virtual ~HNWithTitle() noexcept = default;

protected:
    friend class HNClient;
    std::string m_title;
};

#endif // HNWITHTITLE_H
