#include <CZ/Heaven/Client/HNClient.h>
#include <CZ/Heaven/Client/HNWithParent.h>
#include <CZ/Heaven/Client/HNWithChildren.h>

using namespace CZ;
using namespace CZ::HNClientAPI;

CZ::HNClientAPI::HNWithChildren::~HNWithChildren() noexcept
{
    while (!m_children.empty())
        m_children.back()->setParent(nullptr);
}
