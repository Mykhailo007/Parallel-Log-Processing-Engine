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