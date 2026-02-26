#include "parser.h"
#include <ctime>

namespace logengine
{

  std::optional<LogEntry> Parser::parse_line(std::string_view line)
  {
    if (line.empty())
    {
      return std::nullopt;
    }

    LogEntry entry;
    size_t pos = 0;

    // Simple key=value parser
    while (pos < line.size())
    {
      // Skip whitespace
      while (pos < line.size() && std::isspace(line[pos]))
        pos++;
      if (pos >= line.size())
        break;

      // Find key
      size_t key_start = pos;
      while (pos < line.size() && line[pos] != '=')
        pos++;
      if (pos >= line.size())
        break;

      std::string_view key(line.data() + key_start, pos - key_start);
      pos++; // Skip '='

      // Find value
      size_t value_start = pos;
      while (pos < line.size() && !std::isspace(line[pos]))
        pos++;
      std::string_view value(line.data() + value_start, pos - value_start);

      // Parse fields
      if (key == "ts")
      {
        auto ts = parse_timestamp(value);
        if (ts.has_value())
        {
          entry.timestamp = ts.value();
        }
      }
      else if (key == "level")
      {
        entry.level = std::string(value);
      }
      else if (key == "service")
      {
        entry.service = std::string(value);
      }
      else if (key == "endpoint")
      {
        entry.endpoint = std::string(value);
      }
      else if (key == "status")
      {
        entry.status = std::stoi(std::string(value));
      }
      else if (key == "latency_ms")
      {
        entry.latency_ms = std::stoi(std::string(value));
      }
    }

    return entry;
  }

  std::optional<std::chrono::system_clock::time_point> Parser::parse_timestamp(std::string_view ts_str)
  {
    // Manual UTC parsing to avoid mktime()'s repeated stat("/etc/localtime") syscalls.
    // Input format: 2026-02-26T10:00:00Z (ISO8601 UTC, Z suffix required)
    if (ts_str.size() != 20 || ts_str[19] != 'Z')
      return std::nullopt;

    std::tm tm = {};

    auto parse_int = [](std::string_view sv) -> int {
      int val = 0;
      for (char c : sv) {
        if (c < '0' || c > '9') return -1;
        val = val * 10 + (c - '0');
      }
      return val;
    };

    tm.tm_year = parse_int(ts_str.substr(0, 4)) - 1900;
    tm.tm_mon  = parse_int(ts_str.substr(5, 2)) - 1;
    tm.tm_mday = parse_int(ts_str.substr(8, 2));
    tm.tm_hour = parse_int(ts_str.substr(11, 2));
    tm.tm_min  = parse_int(ts_str.substr(14, 2));
    tm.tm_sec  = parse_int(ts_str.substr(17, 2));

    if (tm.tm_year < 0 || tm.tm_mon < 0 || tm.tm_mday < 1 ||
        tm.tm_hour < 0 || tm.tm_min < 0 || tm.tm_sec < 0)
      return std::nullopt;

    // Convert to UTC time_point WITHOUT calling mktime (which stats /etc/localtime)
#ifdef _WIN32
    auto epoch_seconds = _mkgmtime(&tm);
#else
    auto epoch_seconds = timegm(&tm);  // POSIX: UTC conversion, no stat syscalls
#endif

    if (epoch_seconds == -1)
      return std::nullopt;

    return std::chrono::system_clock::from_time_t(epoch_seconds);
  }

} // namespace logengine