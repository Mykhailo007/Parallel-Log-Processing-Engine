#include <doctest/doctest.h>
#include "engine/engine.h"
#include <fstream>
#include <filesystem>

using namespace logengine;

TEST_CASE("Engine - Process small file")
{
  // Create temporary test file
  std::string test_file = "/tmp/test_engine.log";
  {
    std::ofstream out(test_file);
    out << "ts=2026-02-26T10:00:00Z level=INFO service=api endpoint=/login status=200 latency_ms=12\n";
    out << "ts=2026-02-26T10:00:01Z level=ERROR service=db endpoint=/query status=500 latency_ms=1200\n";
    out << "ts=2026-02-26T10:00:02Z level=INFO service=api endpoint=/login status=200 latency_ms=8\n";
  }

  Engine engine(2);
  auto metrics = engine.process_file(test_file);

  CHECK(metrics.total_lines == 3);
  CHECK(metrics.error_count == 1);
  CHECK(metrics.requests_per_endpoint["/login"] == 2);
  CHECK(metrics.requests_per_endpoint["/query"] == 1);

  std::filesystem::remove(test_file);
}

TEST_CASE("Engine - Parallel processing consistency")
{
  std::string test_file = "/tmp/test_parallel.log";
  {
    std::ofstream out(test_file);
    for (int i = 0; i < 1000; i++)
    {
      out << "ts=2026-02-26T10:00:00Z level=INFO service=api endpoint=/test status=200 latency_ms=10\n";
    }
  }

  Engine engine1(1);
  auto metrics1 = engine1.process_file(test_file);

  Engine engine2(4);
  auto metrics2 = engine2.process_file(test_file);

  CHECK(metrics1.total_lines == metrics2.total_lines);
  CHECK(metrics1.error_count == metrics2.error_count);

  std::filesystem::remove(test_file);
}

TEST_CASE("Engine - Parallel correctness: 1 vs 4 threads")
{
  std::string test_file = "/tmp/test_parallel_correctness.log";
  {
    std::ofstream out(test_file);
    // Mix of endpoints, statuses, and latencies across multiple time windows
    for (int i = 0; i < 500; i++)
    {
      out << "ts=2026-02-26T10:00:00Z level=INFO service=api endpoint=/login status=200 latency_ms=12\n";
      out << "ts=2026-02-26T10:01:00Z level=ERROR service=db endpoint=/query status=500 latency_ms=900\n";
      out << "ts=2026-02-26T10:02:00Z level=INFO service=api endpoint=/health status=200 latency_ms=3\n";
      out << "ts=2026-02-26T10:03:00Z level=WARN service=api endpoint=/login status=429 latency_ms=50\n";
    }
  }

  Engine engine1(1);
  auto metrics1 = engine1.process_file(test_file);

  Engine engine4(4);
  auto metrics4 = engine4.process_file(test_file);

  // Exact line and error counts must match
  CHECK(metrics1.total_lines == metrics4.total_lines);
  CHECK(metrics1.error_count == metrics4.error_count);

  // Endpoint request counts must match exactly
  for (const auto &[endpoint, count] : metrics1.requests_per_endpoint)
  {
    CHECK(metrics4.requests_per_endpoint.count(endpoint) == 1);
    CHECK(metrics4.requests_per_endpoint.at(endpoint) == count);
  }

  // Percentiles must match (same latencies, just different order of merging)
  auto p1 = metrics1.calculate_percentiles();
  auto p4 = metrics4.calculate_percentiles();
  CHECK(p1.p50 == p4.p50);
  CHECK(p1.p95 == p4.p95);
  CHECK(p1.p99 == p4.p99);

  // Time window bucket counts must match
  CHECK(metrics1.time_windows.size() == metrics4.time_windows.size());

  std::filesystem::remove(test_file);
}