#ifndef HNTOGGLE_H
#define HNTOGGLE_H

#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithTitle.h>
#include <CZ/Heaven/Client/HNWithIcon.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>

/**
 * @brief Clickable item with a checked/unchecked state created by a client.
 *
 * As with actions, clicks arrive through HNObject::onClicked. The client is
 * responsible for updating the checked state (typically toggling it) in
 * response and committing the change.
 */
class CZ::Client::HNToggle :
    public HNObject,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithEnabled,
    public HNWithParent
{
public:
    /**
     * @brief Creates a new toggle and applies the given initial properties.
     *
     * @param title    Initial title.
     * @param icon     Initial icon name.
     * @param shortcut Initial keyboard shortcut.
     * @param checked  Initial checked state.
     * @param enabled  Initial enabled state.
     * @param parent   Object to attach this toggle to, or nullptr.
     * @return Shared pointer to the new toggle, or nullptr on failure.
     */
    static std::shared_ptr<HNToggle> Make(
        const std::string &title = "",
        const std::string &icon = "",
        const std::string &shortcut = "",
        bool checked = false,
        bool enabled = true,
        HNObject *parent = nullptr) noexcept;

    /**
     * @brief Updates the checked state and notifies the bar.
     *
     * @param checked New checked state.
     */
    void setChecked(bool checked) noexcept;

    /**
     * @brief Returns the current checked state.
     *
     * @return true if checked, false otherwise.
     */
    bool checked() const noexcept { return m_checked; }

private:
    HNToggle(std::shared_ptr<HNClient> client, UInt32 id) noexcept :
        HNObject(client, id, Type::Toggle) {}
    bool m_checked { false };
};

#endif // HNTOGGLE_H
