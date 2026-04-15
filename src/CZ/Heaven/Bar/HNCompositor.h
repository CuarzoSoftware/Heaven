#ifndef HNCOMPOSITOR_H
#define HNCOMPOSITOR_H

#include <CZ/Heaven/Heaven.h>
#include <CZ/Core/CZObject.h>
#include <string>

class CZ::Bar::HNCompositor : public CZObject
{
public:
    const std::string &id() const noexcept { return m_id; }
    ~HNCompositor() noexcept = default;
private:
    friend struct HNIface;
    friend class HNBar;
    HNCompositor(const std::string &id) noexcept : m_id(id) {}
    std::string m_id;
};

#endif // HNCOMPOSITOR_H
