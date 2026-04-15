#include <CZ/Heaven/Compositor/HNLog.h>

using namespace CZ;

const CZ::CZLogger &HNCompositorLogger() noexcept
{
    static CZLogger logger { "Heaven Compositor", "CZ_HEAVEN_COMPOSITOR_LOG_LEVEL" };
    return logger;
}
