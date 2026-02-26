#pragma once
#include "metrics.h"
#include "parser/parser.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <cstdint>

namespace logengine
{

  // Per-thread accumulator: each thread writes only to its own copy,
  // eliminating mutex/atomic contention on the hot parse path.
  struct ThreadLocalMetrics {
    size_t local_total_lines = 0;
    size_t local_error_count = 0;
    std::unordered_map<std::string, uint32_t> local_requests_per_endpoint;
    std::unordered_map<uint64_t, uint32_t> local_time_windows; // minute bucket -> count
    std::vector<int> local_latencies;
  };

  class Aggregator
  {
  public:
    void add_entry(const LogEntry &entry);
    void merge(const Aggregator &other);
    // Merge a thread-local result into the final aggregator (post-parallel phase)
    void merge_thread_local(const ThreadLocalMetrics &local);
    Metrics get_metrics() const;

    // Convert a time_point to an integer minute bucket (thread-safe, no allocation)
    static uint64_t time_point_to_minute_bucket(
        const std::chrono::system_clock::time_point &tp);

  private:
    size_t total_lines_ = 0;
    size_t error_count_ = 0;

    std::map<std::string, size_t> endpoint_counts_;
    std::vector<int> latencies_;
    std::map<std::chrono::system_clock::time_point, size_t> time_windows_;

    std::chrono::system_clock::time_point truncate_to_minute(
        const std::chrono::system_clock::time_point &tp);
  };

} // namespace logengine