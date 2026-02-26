#include "metrics.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace logengine
{

  Metrics::Percentiles Metrics::calculate_percentiles() const
  {
    Percentiles p;
    if (latencies.empty())
      return p;

    std::vector<int> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());

    auto percentile = [&](double pct)
    {
      size_t idx = static_cast<size_t>(pct * sorted.size());
      if (idx >= sorted.size())
        idx = sorted.size() - 1;
      return static_cast<double>(sorted[idx]);
    };

    p.p50 = percentile(0.50);
    p.p95 = percentile(0.95);
    p.p99 = percentile(0.99);

    return p;
  }

  std::string Metrics::to_text() const
  {
    std::ostringstream oss;

    oss << "=== Log Analysis Results ===\n\n";

    // Top endpoints first (useful signal up-front)
    oss << "Top 10 Endpoints:\n";
    std::vector<std::pair<std::string, size_t>> sorted_endpoints(
        requests_per_endpoint.begin(), requests_per_endpoint.end());
    std::sort(sorted_endpoints.begin(), sorted_endpoints.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });

    for (size_t i = 0; i < std::min<size_t>(10, sorted_endpoints.size()); i++)
    {
      oss << "  " << std::setw(20) << std::left << sorted_endpoints[i].first
          << " : " << sorted_endpoints[i].second << "\n";
    }

    // Percentiles next
    auto pct = calculate_percentiles();
    oss << "\nLatency Percentiles (ms):\n";
    oss << std::fixed << std::setprecision(2);
    oss << "  P50: " << pct.p50 << "\n";
    oss << "  P95: " << pct.p95 << "\n";
    oss << "  P99: " << pct.p99 << "\n";

    // Big section: time windows
    oss << "\nTime Window Summary (requests per minute):\n";
    for (const auto &[tp, count] : time_windows)
    {
      auto tt = std::chrono::system_clock::to_time_t(tp);
      char buf[32];
      std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::gmtime(&tt));
      oss << "  " << buf << " : " << count << "\n";
    }

    // Summary at the bottom so it doesn't scroll away
    const double err_pct = (total_lines > 0) ? (100.0 * static_cast<double>(error_count) / static_cast<double>(total_lines)) : 0.0;

    oss << "\n=== Summary ===\n";
    oss << "Total Lines: " << total_lines << "\n";
    oss << "Error Count: " << error_count << " ("
        << std::fixed << std::setprecision(2) << err_pct << "%)\n";

    // Compact final line for quick reading
    oss << "\n--- Final Stats ---\n";
    oss << "Lines: " << total_lines
        << " | Errors: " << error_count
        << " | P95: " << std::fixed << std::setprecision(2) << pct.p95 << " ms\n";

    return oss.str();
  }

  std::string Metrics::to_json() const
  {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"total_lines\": " << total_lines << ",\n";
    oss << "  \"error_count\": " << error_count << ",\n";

    oss << "  \"top_endpoints\": [\n";
    std::vector<std::pair<std::string, size_t>> sorted_endpoints(
        requests_per_endpoint.begin(), requests_per_endpoint.end());
    std::sort(sorted_endpoints.begin(), sorted_endpoints.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });

    for (size_t i = 0; i < std::min<size_t>(10, sorted_endpoints.size()); i++)
    {
      oss << "    {\"endpoint\": \"" << sorted_endpoints[i].first
          << "\", \"count\": " << sorted_endpoints[i].second << "}";
      if (i < std::min<size_t>(10, sorted_endpoints.size()) - 1)
        oss << ",";
      oss << "\n";
    }
    oss << "  ],\n";

    auto pct = calculate_percentiles();
    oss << "  \"latency_percentiles\": {\n";
    oss << "    \"p50\": " << std::fixed << std::setprecision(2) << pct.p50 << ",\n";
    oss << "    \"p95\": " << pct.p95 << ",\n";
    oss << "    \"p99\": " << pct.p99 << "\n";
    oss << "  }\n";
    oss << "}\n";

    return oss.str();
  }

} // namespace logengine