#ifndef HNDIVIDER_H
#define HNDIVIDER_H

#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithTitle.h>

/**
 * @brief Non-interactive separator created by a client.
 *
 * A divider can optionally carry a title, which the bar may render as a
 * section label.
 */
class CZ::Client::HNDivider :
    public HNObject,
    public HNWithTitle,
    public HNWithParent
{
public:
    /**
     * @brief Creates a new divider.
     *
     * @param title  Optional section title.
     * @param parent Object to attach this divider to, or nullptr.
     * @return Shared pointer to the new divider, or nullptr on failure.
     */
    static std::shared_ptr<HNDivider> Make(
        const std::string &title = "",
        HNObject *parent = nullptr) noexcept;

private:
    HNDivider(std::shared_ptr<HNClient> client, UInt32 id) noexcept :
        HNObject(client, id, Type::Divider) {}
};

#endif // HNDIVIDER_H
