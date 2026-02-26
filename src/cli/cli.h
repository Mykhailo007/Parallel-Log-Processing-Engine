#pragma once
#include <string>
#include <vector>

namespace logengine
{

  struct AnalyzeOptions
  {
    std::string input_file;
    std::string output_format = "text"; // text or json
    size_t num_threads = 4;
    bool use_mmap = false;
  };

  struct BenchOptions
  {
    std::string input_file;
    std::vector<size_t> thread_counts;
    size_t repeat = 3;
  };

  struct GenOptions
  {
    std::string output_file;
    size_t num_lines = 1000000;
    size_t num_endpoints = 50;
    double error_rate = 0.01;
  };

  class CLI
  {
  public:
    int run(int argc, char *argv[]);

  private:
    void print_usage(const char *prog_name);
    int cmd_analyze(const AnalyzeOptions &opts);
    int cmd_bench(const BenchOptions &opts);
    int cmd_gen(const GenOptions &opts);
  };

} // namespace logengine