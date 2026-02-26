#include "engine.h"
#include "parser/parser.h"
#include "aggregator/aggregator.h"
#include "utils/file_reader.h"
#include <algorithm>
#include <vector>
#include <thread>

namespace logengine
{

  Engine::Engine(size_t num_threads) : num_threads_(num_threads) {}

  // Divide buffer into at most num_parts partitions, each ending on a newline boundary.
  std::vector<std::string_view> Engine::partition_by_lines(
      const std::string &buffer, size_t num_parts)
  {
    std::vector<std::string_view> partitions;
    if (buffer.empty() || num_parts == 0)
      return partitions;

    const size_t total = buffer.size();
    const size_t base = total / num_parts;

    size_t start = 0;
    for (size_t i = 0; i < num_parts; i++)
    {
      if (start >= total)
        break;

      size_t end;
      if (i == num_parts - 1)
      {
        // Last partition takes the remainder
        end = total;
      }
      else
      {
        end = start + base;
        // Advance to the next newline so we never split a log line
        while (end < total && buffer[end] != '\n')
          end++;
        if (end < total)
          end++; // include the '\n'
      }

      partitions.emplace_back(buffer.data() + start, end - start);
      start = end;
    }
    return partitions;
  }

  Metrics Engine::process_file(const std::string &file_path)
  {
    // Read the entire file into a single buffer - one sequential I/O pass
    std::string file_buffer = FileReader::read_file(file_path);

    // Partition the buffer into exactly num_threads_ slices aligned to newlines
    auto partitions = partition_by_lines(file_buffer, num_threads_);
    const size_t num_partitions = partitions.size();

    // Pre-allocate thread-local storage (no dynamic allocation in hot path)
    std::vector<ThreadLocalMetrics> thread_locals(num_partitions);
    std::vector<std::thread> workers;
    workers.reserve(num_partitions);

    const auto epoch = std::chrono::system_clock::time_point{};

    // Launch one thread per partition - NO shared queue, NO mutex, NO futex
    for (size_t i = 0; i < num_partitions; i++)
    {
      workers.emplace_back([&local = thread_locals[i], partition = partitions[i], epoch]()
      {
        Parser parser;

        const char *data = partition.data();
        const size_t size = partition.size();
        size_t start = 0;
        size_t pos = 0;

        auto process_line = [&](std::string_view line) {
          auto entry = parser.parse_line(line);
          if (!entry.has_value())
            return;
          const auto &e = entry.value();
          local.local_total_lines++;
          if (e.is_error())
            local.local_error_count++;
          if (!e.endpoint.empty())
            local.local_requests_per_endpoint[e.endpoint]++;
          if (e.latency_ms > 0)
            local.local_latencies.push_back(e.latency_ms);
          // Skip bogus epoch timestamps produced by parse failures
          if (e.timestamp != epoch)
          {
            uint64_t bucket = Aggregator::time_point_to_minute_bucket(e.timestamp);
            local.local_time_windows[bucket]++;
          }
        };

        while (pos < size)
        {
          if (data[pos] == '\n')
          {
            if (pos > start)
              process_line({data + start, pos - start});
            start = pos + 1;
          }
          pos++;
        }
        // Handle last line if partition has no trailing newline
        if (start < size)
          process_line({data + start, size - start});
      });
    }

    // Join all workers (no futex contention - threads simply finish)
    for (auto &worker : workers)
    {
      worker.join();
    }

    // Single-threaded merge phase: aggregate all thread-local results
    Aggregator final_agg;
    for (const auto &local : thread_locals)
    {
      final_agg.merge_thread_local(local);
    }

    return final_agg.get_metrics();
  }

} // namespace logengine