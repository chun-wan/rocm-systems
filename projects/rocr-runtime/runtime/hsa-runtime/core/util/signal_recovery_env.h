////////////////////////////////////////////////////////////////////////////////
//
// Optional bounded wait / hang recovery for in-process signal waits.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef HSA_RUNTIME_CORE_UTIL_SIGNAL_RECOVERY_ENV_H_
#define HSA_RUNTIME_CORE_UTIL_SIGNAL_RECOVERY_ENV_H_

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>

#include "core/util/timer.h"
#include "inc/hsa.h"

namespace rocr {
namespace core {

/// Reads HSA_MAX_SIGNAL_WAIT_SEC once (cached). 0 = disabled (default).
/// Unlike HSA_SIGNAL_WAIT_ABORT_TIMEOUT, this path does not throw; it forces
/// the signal value so the wait can return (best-effort recovery on stuck GPU).
struct SignalRecoveryEnv {
  static uint32_t MaxSignalWaitSec() {
    static constexpr uint32_t kUnset = 0xFFFFFFFFu;
    static std::atomic<uint32_t> cached{kUnset};
    uint32_t v = cached.load(std::memory_order_relaxed);
    if (v != kUnset) return v;
    uint32_t parsed = 0;
    const char* e = getenv("HSA_MAX_SIGNAL_WAIT_SEC");
    if (e && e[0] != '\0') {
      char* end = nullptr;
      unsigned long ul = strtoul(e, &end, 10);
      if (end != e && *end == '\0' && ul <= 604800UL) parsed = static_cast<uint32_t>(ul);
    }
    cached.store(parsed, std::memory_order_relaxed);
    return parsed;
  }
};

inline bool ShouldForceUnblockSignalWait(
    const timer::fast_clock::time_point& start_time, uint32_t max_sec) {
  if (max_sec == 0) return false;
  return (timer::fast_clock::now() - start_time) >= std::chrono::seconds(max_sec);
}

/// Pick a host-side value that satisfies the wait condition (for recovery only).
inline int64_t RecoveryValueToSatisfyWait(hsa_signal_condition_t condition,
                                          hsa_signal_value_t compare_value,
                                          int64_t current) {
  const int64_t c = int64_t(compare_value);
  switch (condition) {
    case HSA_SIGNAL_CONDITION_EQ:
      return c;
    case HSA_SIGNAL_CONDITION_NE:
      if (current != c) return current;
      if (c == std::numeric_limits<int64_t>::max()) return c - 1;
      return c + 1;
    case HSA_SIGNAL_CONDITION_GTE:
      return (std::max)(current, c);
    case HSA_SIGNAL_CONDITION_LT:
      if (current < c) return current;
      if (c == std::numeric_limits<int64_t>::min()) return c;
      return c - 1;
    default:
      return c;
  }
}

}  // namespace core
}  // namespace rocr

#endif  // HSA_RUNTIME_CORE_UTIL_SIGNAL_RECOVERY_ENV_H_
