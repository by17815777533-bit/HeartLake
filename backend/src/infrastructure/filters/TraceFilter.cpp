/**
 * @file TraceFilter.cpp
 * @brief TraceFilter 实现
 */

#include "infrastructure/filters/TraceFilter.h"

namespace heartlake {
namespace filters {

thread_local std::string TraceContext::traceId_;
thread_local std::string TraceContext::spanId_;

} // namespace filters
} // namespace heartlake
