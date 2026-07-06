#ifndef HNACTION_H
#define HNACTION_H

#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithTitle.h>
#include <CZ/Heaven/Client/HNWithIcon.h>
#include <CZ/Heaven/Client/HNWithShortcut.h>
#include <CZ/Heaven/Client/HNWithEnabled.h>

/**
 * @brief Clickable action item created by a client.
 *
 * When the user activates an action in the bar, the client receives a click
 * notification through HNObject::onClicked.
 */
class CZ::Client::HNAction :
    public HNObject,
    public HNWithParent,
    public HNWithTitle,
    public HNWithIcon,
    public HNWithShortcut,
    public HNWithEnabled
{
public:
    /**
     * @brief Creates a new action and applies the given initial properties.
     *
     * @param title    Initial title.
     * @param icon     Initial icon name.
     * @param shortcut Initial keyboard shortcut.
     * @param enabled  Initial enabled state.
     * @param parent   Object to attach this action to, or nullptr.
     * @return Shared pointer to the new action, or nullptr on failure.
     */
    static std::shared_ptr<HNAction> Make(
        const std::string &title = "",
        const std::string &icon = "",
        const std::string &shortcut = "",
        bool enabled = true,
        HNObject *parent = nullptr) noexcept;

private:
    HNAction(std::shared_ptr<HNClient> client, UInt32 id) noexcept :
        HNObject(client, id, Type::Action) {}
};

#endif // HNACTION_H
