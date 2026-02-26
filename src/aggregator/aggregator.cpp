#include "aggregator.h"
#include <algorithm>

namespace logengine
{

  void Aggregator::add_entry(const LogEntry &entry)
  {
    total_lines_++;

    if (entry.is_error())
    {
      error_count_++;
    }

    if (!entry.endpoint.empty())
    {
      endpoint_counts_[entry.endpoint]++;
    }

    if (entry.latency_ms > 0)
    {
      latencies_.push_back(entry.latency_ms);
    }

    auto minute = truncate_to_minute(entry.timestamp);
    time_windows_[minute]++;
  }

  void Aggregator::merge(const Aggregator &other)
  {
    total_lines_ += other.total_lines_;
    error_count_ += other.error_count_;

    for (const auto &[endpoint, count] : other.endpoint_counts_)
    {
      endpoint_counts_[endpoint] += count;
    }

    latencies_.insert(latencies_.end(), other.latencies_.begin(), other.latencies_.end());

    for (const auto &[time, count] : other.time_windows_)
    {
      time_windows_[time] += count;
    }
  }

  Metrics Aggregator::get_metrics() const
  {
    Metrics m;
    m.total_lines = total_lines_;
    m.error_count = error_count_;
    m.requests_per_endpoint = endpoint_counts_;
    m.latencies = latencies_;
    m.time_windows = time_windows_;
    return m;
  }

  std::chrono::system_clock::time_point Aggregator::truncate_to_minute(
      const std::chrono::system_clock::time_point &tp)
  {
    auto duration = tp.time_since_epoch();
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    return std::chrono::system_clock::time_point(minutes);
  }

  uint64_t Aggregator::time_point_to_minute_bucket(
      const std::chrono::system_clock::time_point &tp)
  {
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(
        tp.time_since_epoch());
    return static_cast<uint64_t>(minutes.count());
  }

  void Aggregator::merge_thread_local(const ThreadLocalMetrics &local)
  {
    total_lines_ += local.local_total_lines;
    error_count_ += local.local_error_count;

    for (const auto &[endpoint, count] : local.local_requests_per_endpoint)
    {
      endpoint_counts_[endpoint] += count;
    }

    latencies_.insert(latencies_.end(),
                      local.local_latencies.begin(),
                      local.local_latencies.end());

    // Convert integer minute buckets back to time_points for the final map
    for (const auto &[bucket, count] : local.local_time_windows)
    {
      auto tp = std::chrono::system_clock::time_point(
          std::chrono::minutes(static_cast<int64_t>(bucket)));
      time_windows_[tp] += count;
    }
  }

} // namespace logengine