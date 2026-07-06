#ifndef HNCOMPOSITOR_H
#define HNCOMPOSITOR_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <string>

/**
 * @brief Represents, on the bar side, the connected Wayland compositor.
 *
 * The bar keeps a single HNCompositor instance while the compositor is present
 * on the bus. It is created and destroyed automatically as the compositor
 * appears and disappears (see HNBar::onCompositorChanged).
 */
class CZ::Bar::HNCompositor : public CZObject
{
public:
    /**
     * @brief Returns the D-Bus unique name of the compositor.
     *
     * @return Const reference to the compositor's D-Bus id.
     */
    const std::string &id() const noexcept { return m_id; }

    /**
     * @brief Destructor.
     */
    ~HNCompositor() noexcept = default;

private:
    friend struct HNIface;
    friend class HNBar;
    HNCompositor(const std::string &id) noexcept : m_id(id) {}
    std::string m_id;
};

#endif // HNCOMPOSITOR_H
