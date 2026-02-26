#pragma once
#include "metrics.h"
#include "parser/parser.h"
#include <map>
#include <vector>
#include <string>
#include <chrono>

namespace logengine
{

  class Aggregator
  {
  public:
    void add_entry(const LogEntry &entry);
    void merge(const Aggregator &other);
    Metrics get_metrics() const;

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