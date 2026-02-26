cmake_minimum_required(VERSION 3.16)
project(ParallelLogProcessingEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compile options
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address -fsanitize=undefined")

# Find threading library
find_package(Threads REQUIRED)

# Include FetchContent for doctest
include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

# Source files
set(ENGINE_SOURCES
    src/parser/parser.cpp
    src/aggregator/aggregator.cpp
    src/aggregator/metrics.cpp
    src/engine/engine.cpp
    src/engine/thread_pool.cpp
    src/cli/cli.cpp
)

# Main executable
add_executable(logengine 
    src/main.cpp
    ${ENGINE_SOURCES}
)

target_include_directories(logengine PRIVATE src)
target_link_libraries(logengine PRIVATE Threads::Threads)

# Tests
add_executable(logengine_tests
    tests/test_main.cpp
    tests/test_parser.cpp
    tests/test_aggregator.cpp
    tests/test_engine.cpp
    ${ENGINE_SOURCES}
)

target_include_directories(logengine_tests PRIVATE src)
target_link_libraries(logengine_tests PRIVATE Threads::Threads doctest::doctest)

enable_testing()
add_test(NAME UnitTests COMMAND logengine_tests)