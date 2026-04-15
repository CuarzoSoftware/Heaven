#include <CZ/Heaven/Bar/HNLog.h>

using namespace CZ;

const CZ::CZLogger &HNLogger() noexcept
{
    static CZLogger logger { "Heaven Bar", "CZ_HEAVEN_BAR_LOG_LEVEL" };
    return logger;
}
