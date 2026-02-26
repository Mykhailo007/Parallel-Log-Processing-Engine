#include <doctest/doctest.h>
#include "aggregator/aggregator.h"

using namespace logengine;

TEST_CASE("Aggregator - Basic aggregation")
{
  Aggregator agg;

  LogEntry entry1;
  entry1.level = "INFO";
  entry1.endpoint = "/login";
  entry1.status = 200;
  entry1.latency_ms = 10;
  entry1.timestamp = std::chrono::system_clock::now();

  LogEntry entry2;
  entry2.level = "ERROR";
  entry2.endpoint = "/api";
  entry2.status = 500;
  entry2.latency_ms = 1000;
  entry2.timestamp = std::chrono::system_clock::now();

  agg.add_entry(entry1);
  agg.add_entry(entry2);

  auto metrics = agg.get_metrics();

  CHECK(metrics.total_lines == 2);
  CHECK(metrics.error_count == 1);
  CHECK(metrics.requests_per_endpoint["/login"] == 1);
  CHECK(metrics.requests_per_endpoint["/api"] == 1);
  CHECK(metrics.latencies.size() == 2);
}

TEST_CASE("Aggregator - Merge")
{
  Aggregator agg1, agg2;

  LogEntry entry1;
  entry1.level = "INFO";
  entry1.endpoint = "/test";
  entry1.status = 200;
  entry1.latency_ms = 5;
  entry1.timestamp = std::chrono::system_clock::now();

  agg1.add_entry(entry1);
  agg2.add_entry(entry1);

  agg1.merge(agg2);
  auto metrics = agg1.get_metrics();

  CHECK(metrics.total_lines == 2);
  CHECK(metrics.requests_per_endpoint["/test"] == 2);
}

TEST_CASE("Metrics - Percentiles")
{
  Metrics m;
  m.latencies = {1, 5, 10, 20, 50, 100, 200, 500, 1000};

  auto pct = m.calculate_percentiles();

  CHECK(pct.p50 > 0);
  CHECK(pct.p95 > pct.p50);
  CHECK(pct.p99 > pct.p95);
}