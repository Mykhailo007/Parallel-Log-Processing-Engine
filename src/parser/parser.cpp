#include "parser.h"
#include <sstream>
#include <iomanip>
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
    std::tm tm = {};
    std::string ts_string(ts_str);

    // Parse ISO8601 format: 2026-02-26T10:00:00Z
    std::istringstream ss(ts_string);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    if (ss.fail())
    {
      return std::nullopt;
    }

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return tp;
  }

} // namespace logengine