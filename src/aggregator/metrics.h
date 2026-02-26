#pragma once
#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace logengine
{

  struct Metrics
  {
    size_t total_lines = 0;
    size_t error_count = 0;

    std::map<std::string, size_t> requests_per_endpoint;
    std::vector<int> latencies; // For percentile calculation

    // Time window: minute -> count
    std::map<std::chrono::system_clock::time_point, size_t> time_windows;

    struct Percentiles
    {
      double p50 = 0.0;
      double p95 = 0.0;
      double p99 = 0.0;
    };

    Percentiles calculate_percentiles() const;
    std::string to_text() const;
    std::string to_json() const;
  };

} // namespace logengine