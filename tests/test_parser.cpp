#include <doctest/doctest.h>
#include "parser/parser.h"

using namespace logengine;

TEST_CASE("Parser - Valid line")
{
  Parser parser;
  auto result = parser.parse_line("ts=2026-02-26T10:00:00Z level=INFO service=api endpoint=/login status=200 latency_ms=12");

  REQUIRE(result.has_value());
  CHECK(result->level == "INFO");
  CHECK(result->service == "api");
  CHECK(result->endpoint == "/login");
  CHECK(result->status == 200);
  CHECK(result->latency_ms == 12);
  CHECK_FALSE(result->is_error());
}

TEST_CASE("Parser - Error line")
{
  Parser parser;
  auto result = parser.parse_line("ts=2026-02-26T10:00:00Z level=ERROR service=db endpoint=/query status=500 latency_ms=1200");

  REQUIRE(result.has_value());
  CHECK(result->level == "ERROR");
  CHECK(result->status == 500);
  CHECK(result->is_error());
}

TEST_CASE("Parser - Missing fields")
{
  Parser parser;
  auto result = parser.parse_line("ts=2026-02-26T10:00:00Z level=INFO endpoint=/test");

  REQUIRE(result.has_value());
  CHECK(result->level == "INFO");
  CHECK(result->endpoint == "/test");
  CHECK(result->status == 0);
}

TEST_CASE("Parser - Empty line")
{
  Parser parser;
  auto result = parser.parse_line("");
  CHECK_FALSE(result.has_value());
}