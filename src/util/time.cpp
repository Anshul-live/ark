#include <ark/util/time.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ark {

std::string formatLocalUtcOffset() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);

  std::tm utc_tm = *std::gmtime(&t);
  std::tm local_tm = *std::localtime(&t);

  auto diff = std::mktime(&local_tm) - std::mktime(&utc_tm);

  int hours = diff / 3600;
  int minutes = (std::abs(diff) % 3600) / 60;

  std::ostringstream oss;
  oss << (diff >= 0 ? '+' : '-') << std::setw(2) << std::setfill('0')
      << std::abs(hours) << std::setw(2) << std::setfill('0') << minutes;

  return oss.str();
}

} // namespace ark
