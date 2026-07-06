#ifndef HNTOPBAR_H
#define HNTOPBAR_H

#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithChildren.h>

/**
 * @brief Top bar container created by a client.
 *
 * A topbar hosts menus and represents the menu of a single window. A client may
 * create several topbars (e.g. one per window) and use HNClient::setActiveTopbar()
 * to tell the bar which one should currently be displayed.
 */
class CZ::Client::HNTopbar :
    public HNObject,
    public HNWithChildren
{
public:
    /**
     * @brief Creates a new topbar.
     *
     * @return Shared pointer to the new topbar, or nullptr on failure
     *         (no client instance or object id limit reached).
     */
    static std::shared_ptr<HNTopbar> Make() noexcept;

private:
    HNTopbar(std::shared_ptr<HNClient> client, UInt32 id) noexcept :
        HNObject(client, id, Type::Topbar) {}
};

#endif // HNTOPBAR_H
