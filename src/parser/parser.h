#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <chrono>

namespace logengine
{

  struct LogEntry
  {
    std::chrono::system_clock::time_point timestamp;
    std::string level;
    std::string service;
    std::string endpoint;
    int status = 0;
    int latency_ms = 0;

    bool is_error() const
    {
      return status >= 500 || level == "ERROR";
    }
  };

  class Parser
  {
  public:
    std::optional<LogEntry> parse_line(std::string_view line);

  private:
    std::optional<std::chrono::system_clock::time_point> parse_timestamp(std::string_view ts_str);
  };

} // namespace logengine