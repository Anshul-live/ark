#ifndef ARK_UTIL_TIME_H
#define ARK_UTIL_TIME_H

#include <string>

namespace ark {

// Local offset from UTC formatted as +HHMM or -HHMM.
std::string formatLocalUtcOffset();

} // namespace ark

#endif
