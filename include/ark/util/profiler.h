#ifndef ARK_UTIL_PROFILER_H
#define ARK_UTIL_PROFILER_H

// Phase profiler. It is compiled in only when the build defines ARK_PROFILER (CMake option
// ARK_PROFILER=ON). Otherwise every macro below expands to nothing, so a normal build has no
// profiling code at all.
//
// When compiled in, the profiler is off unless ARK_PROFILE is set to a file path. Then each scope
// records how many times it ran and its inclusive time (everything inside it) and exclusive time
// (inclusive minus the scopes nested in it). At exit the totals are appended to that file as TSV:
//   <command>  scope    <name>     <calls>  <inclusive_ns>  <exclusive_ns>
//   <command>  counter  <name>     <value>

#ifdef ARK_PROFILER

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace ark::prof {

bool enabled();

class ScopeTimer {
public:
  explicit ScopeTimer(const char *name);
  ~ScopeTimer();
  ScopeTimer(const ScopeTimer &) = delete;
  ScopeTimer &operator=(const ScopeTimer &) = delete;

private:
  const char *name_;
  bool active_;
  std::chrono::steady_clock::time_point start_;
  ScopeTimer *parent_;
  std::uint64_t childNs_ = 0;
};

void count(const char *name, std::uint64_t amount = 1);

// Writes the accumulated totals for `command` and clears them.
void dump(const std::string &command);

// Dumps the totals when it goes out of scope, so every return path writes the profile.
class Session {
public:
  explicit Session(std::string command) : command_(std::move(command)) {}
  ~Session() { dump(command_); }
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;

private:
  std::string command_;
};

} // namespace ark::prof

#define ARK_PROF_CONCAT_(a, b) a##b
#define ARK_PROF_CONCAT(a, b) ARK_PROF_CONCAT_(a, b)
#define ARK_PROF_SCOPE(name)                                                   \
  ark::prof::ScopeTimer ARK_PROF_CONCAT(arkProfScope_, __LINE__)(name)
#define ARK_PROF_COUNT(name, amount)                                           \
  do {                                                                         \
    if (ark::prof::enabled())                                                  \
      ark::prof::count(name, amount);                                          \
  } while (0)
#define ARK_PROF_SESSION(command) ark::prof::Session arkProfSession(command)

#else

#define ARK_PROF_SCOPE(name) static_cast<void>(0)
#define ARK_PROF_COUNT(name, amount) static_cast<void>(0)
#define ARK_PROF_SESSION(command) static_cast<void>(0)

#endif

#endif
