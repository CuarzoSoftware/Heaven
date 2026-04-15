#ifndef CZ_HNLOG_H
#define CZ_HNLOG_H

#include <CZ/Core/CZLogger.h>

#define HNLog HNCompositorLogger()

const CZ::CZLogger &HNCompositorLogger() noexcept;

#endif // CZ_HNLOG_H
