//
// Created by claude on 10/11/25.
//
#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace epoch_script::common {

/**
 * @brief RAII-based performance timer that logs slow operations
 *
 * Automatically measures elapsed time from construction to destruction.
 * Only logs a warning if the operation exceeds the threshold (default 1s).
 *
 * Example usage:
 * @code
 *   using namespace std::chrono_literals;
 *   {
 *     PerformanceTimer timer("GetStages", 1s,
 *                           std::format("user_id={}, job_id={}", userId, jobId));
 *     auto result = jobManager.GetStages(key);
 *   } // Logs warning if > 1s
 * @endcode
 */
class PerformanceTimer {
public:
  /**
   * @brief Construct a performance timer
   * @param operation_name Name of the operation being timed
   * @param threshold Duration threshold
   * @param context Additional context for logging (e.g., "user_id=x, job_id=y")
   */
  template<typename Rep, typename Period>
  PerformanceTimer(std::string_view operation_name,
                   std::chrono::duration<Rep, Period> threshold,
                   std::string context = "")
      : operation_name_(operation_name),
        threshold_ms_(std::chrono::duration_cast<std::chrono::milliseconds>(threshold).count()),
        context_(std::move(context)),
        start_(std::chrono::steady_clock::now()) {}

  /**
   * @brief Construct a performance timer with default 1s threshold
   * @param operation_name Name of the operation being timed
   * @param context Additional context for logging (e.g., "user_id=x, job_id=y")
   */
  PerformanceTimer(std::string_view operation_name,
                   std::string context = "")
      : operation_name_(operation_name),
        threshold_ms_(1000), // default 1 second
        context_(std::move(context)),
        start_(std::chrono::steady_clock::now()) {}

  /**
   * @brief Destructor - measures elapsed time and logs if threshold exceeded
   */
  ~PerformanceTimer() {
    const auto end = std::chrono::steady_clock::now();
    const auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start_)
            .count();

    if (duration_ms > threshold_ms_) {
      if (context_.empty()) {
        SPDLOG_WARN("Performance: {} took {}ms", operation_name_, duration_ms);
      } else {
        SPDLOG_WARN("Performance: {} took {}ms [{}]", operation_name_,
                    duration_ms, context_);
      }
    }
  }

  // Disable copy and move to prevent misuse
  PerformanceTimer(const PerformanceTimer &) = delete;
  PerformanceTimer &operator=(const PerformanceTimer &) = delete;
  PerformanceTimer(PerformanceTimer &&) = delete;
  PerformanceTimer &operator=(PerformanceTimer &&) = delete;

private:
  std::string_view operation_name_;
  int64_t threshold_ms_;
  std::string context_;
  std::chrono::steady_clock::time_point start_;
};

} // namespace epoch_script::common
