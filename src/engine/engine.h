#pragma once
#include "thread_pool.h"
#include "aggregator/metrics.h"
#include "aggregator/aggregator.h"
#include <string>
#include <string_view>
#include <vector>

namespace logengine
{

  class Engine
  {
  public:
    explicit Engine(size_t num_threads = 4);

    Metrics process_file(const std::string &file_path);

  private:
    size_t num_threads_;

    // Divide buffer into (at most) num_parts partitions aligned to newline boundaries.
    // Each returned string_view is a view into buffer - buffer must outlive the views.
    static std::vector<std::string_view> partition_by_lines(
        const std::string &buffer, size_t num_parts);
  };

} // namespace logengine