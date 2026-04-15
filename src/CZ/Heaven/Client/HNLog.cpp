#include <CZ/Heaven/Client/HNLog.h>

using namespace CZ;

const CZ::CZLogger &HNLogger() noexcept
{
    static CZLogger logger { "Heaven Client", "CZ_HEAVEN_CLIENT_LOG_LEVEL" };
    return logger;
}
