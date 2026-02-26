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

} // namespace logengine