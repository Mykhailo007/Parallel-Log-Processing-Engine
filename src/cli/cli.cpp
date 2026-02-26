#include "cli.h"
#include "engine/engine.h"
#include "utils/timer.h"
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace logengine
{

  void CLI::print_usage(const char *prog_name)
  {
    std::cout << "Usage: " << prog_name << " <command> [options]\n\n"
              << "Commands:\n"
              << "  analyze  - Process and analyze log files\n"
              << "  bench    - Benchmark multi-threaded performance\n"
              << "  gen      - Generate synthetic log data\n\n"
              << "Analyze Options:\n"
              << "  --input <file>       Input log file (required)\n"
              << "  --threads <n>        Number of threads (default: 4)\n"
              << "  --format <fmt>       Output format: text|json (default: text)\n"
              << "  --mmap               Use memory-mapped IO (stub)\n\n"
              << "Bench Options:\n"
              << "  --input <file>       Input log file (required)\n"
              << "  --threads <list>     Comma-separated thread counts (e.g., 1,2,4,8)\n"
              << "  --repeat <n>         Repetitions per config (default: 3)\n\n"
              << "Gen Options:\n"
              << "  --output <file>      Output log file (required)\n"
              << "  --lines <n>          Number of lines (default: 1000000)\n"
              << "  --endpoints <n>      Number of unique endpoints (default: 50)\n"
              << "  --error_rate <f>     Error probability 0-1 (default: 0.01)\n";
  }

  int CLI::run(int argc, char *argv[])
  {
    if (argc < 2)
    {
      print_usage(argv[0]);
      return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "analyze")
    {
      AnalyzeOptions opts;
      for (int i = 2; i < argc; i++)
      {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc)
          opts.input_file = argv[++i];
        else if (arg == "--threads" && i + 1 < argc)
          opts.num_threads = std::stoul(argv[++i]);
        else if (arg == "--format" && i + 1 < argc)
          opts.output_format = argv[++i];
        else if (arg == "--mmap")
          opts.use_mmap = true;
      }
      if (opts.input_file.empty())
      {
        std::cerr << "Error: --input required\n";
        return 1;
      }
      return cmd_analyze(opts);
    }
    else if (cmd == "bench")
    {
      BenchOptions opts;
      for (int i = 2; i < argc; i++)
      {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc)
          opts.input_file = argv[++i];
        else if (arg == "--repeat" && i + 1 < argc)
          opts.repeat = std::stoul(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc)
        {
          std::string list = argv[++i];
          std::stringstream ss(list);
          std::string item;
          while (std::getline(ss, item, ','))
          {
            opts.thread_counts.push_back(std::stoul(item));
          }
        }
      }
      if (opts.input_file.empty() || opts.thread_counts.empty())
      {
        std::cerr << "Error: --input and --threads required\n";
        return 1;
      }
      return cmd_bench(opts);
    }
    else if (cmd == "gen")
    {
      GenOptions opts;
      for (int i = 2; i < argc; i++)
      {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc)
          opts.output_file = argv[++i];
        else if (arg == "--lines" && i + 1 < argc)
          opts.num_lines = std::stoul(argv[++i]);
        else if (arg == "--endpoints" && i + 1 < argc)
          opts.num_endpoints = std::stoul(argv[++i]);
        else if (arg == "--error_rate" && i + 1 < argc)
          opts.error_rate = std::stod(argv[++i]);
      }
      if (opts.output_file.empty())
      {
        std::cerr << "Error: --output required\n";
        return 1;
      }
      return cmd_gen(opts);
    }
    else
    {
      print_usage(argv[0]);
      return 1;
    }
  }

  int CLI::cmd_analyze(const AnalyzeOptions &opts)
  {
    Engine engine(opts.num_threads);

    Timer timer;
    auto metrics = engine.process_file(opts.input_file);
    auto elapsed = timer.elapsed_ms();

    if (opts.output_format == "json")
    {
      std::cout << metrics.to_json() << std::endl;
    }
    else
    {
      std::cout << metrics.to_text() << std::endl;
    }

    std::cout << "\n[Performance]\n"
              << "Processing time: " << elapsed << " ms\n"
              << "Throughput: " << std::fixed << std::setprecision(2)
              << (metrics.total_lines / (elapsed / 1000.0)) << " lines/sec\n";

    return 0;
  }

  int CLI::cmd_bench(const BenchOptions &opts)
  {
    std::cout << "Benchmarking: " << opts.input_file << "\n"
              << "Thread counts: ";
    for (auto t : opts.thread_counts)
      std::cout << t << " ";
    std::cout << "\nRepetitions: " << opts.repeat << "\n\n";

    std::cout << std::setw(10) << "Threads"
              << std::setw(15) << "Time (ms)"
              << std::setw(18) << "Throughput (K/s)"
              << std::setw(12) << "Speedup" << "\n";
    std::cout << std::string(55, '-') << "\n";

    double baseline_time = 0.0;

    for (auto num_threads : opts.thread_counts)
    {
      std::vector<double> times;

      for (size_t rep = 0; rep < opts.repeat; rep++)
      {
        Engine engine(num_threads);
        Timer timer;
        auto metrics = engine.process_file(opts.input_file);
        times.push_back(timer.elapsed_ms());
      }

      // Median time
      std::sort(times.begin(), times.end());
      double median_time = times[times.size() / 2];

      if (num_threads == opts.thread_counts[0])
      {
        baseline_time = median_time;
      }

      Engine temp_engine(num_threads);
      auto metrics = temp_engine.process_file(opts.input_file);
      double throughput = (metrics.total_lines / 1000.0) / (median_time / 1000.0);
      double speedup = baseline_time / median_time;

      std::cout << std::setw(10) << num_threads
                << std::setw(15) << std::fixed << std::setprecision(2) << median_time
                << std::setw(18) << std::fixed << std::setprecision(2) << throughput
                << std::setw(12) << std::fixed << std::setprecision(2) << speedup << "x\n";
    }

    return 0;
  }

  int CLI::cmd_gen(const GenOptions &opts)
  {
    std::ofstream out(opts.output_file);
    if (!out)
    {
      std::cerr << "Error: Cannot open output file: " << opts.output_file << "\n";
      return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> endpoint_dist(0, opts.num_endpoints - 1);
    std::uniform_int_distribution<> latency_dist(1, 100);
    std::uniform_int_distribution<> error_latency_dist(500, 5000);
    std::uniform_real_distribution<> error_dist(0.0, 1.0);
    std::uniform_int_distribution<> status_ok_dist(200, 299);
    std::uniform_int_distribution<> status_err_dist(500, 503);

    const std::vector<std::string> services = {"api", "db", "cache", "auth", "worker"};
    std::uniform_int_distribution<> service_dist(0, services.size() - 1);

    std::cout << "Generating " << opts.num_lines << " lines...\n";

    auto start_time = std::chrono::system_clock::now();

    for (size_t i = 0; i < opts.num_lines; i++)
    {
      auto ts = start_time + std::chrono::seconds(i % 86400); // Cycle within a day
      auto ts_t = std::chrono::system_clock::to_time_t(ts);

      char ts_buf[32];
      std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&ts_t));

      bool is_error = error_dist(gen) < opts.error_rate;
      std::string level = is_error ? "ERROR" : "INFO";
      int status = is_error ? status_err_dist(gen) : status_ok_dist(gen);
      int latency = is_error ? error_latency_dist(gen) : latency_dist(gen);
      std::string service = services[service_dist(gen)];
      std::string endpoint = "/endpoint" + std::to_string(endpoint_dist(gen));

      out << "ts=" << ts_buf
          << " level=" << level
          << " service=" << service
          << " endpoint=" << endpoint
          << " status=" << status
          << " latency_ms=" << latency << "\n";

      if ((i + 1) % 1000000 == 0)
      {
        std::cout << "  " << (i + 1) / 1000000 << "M lines written...\n";
      }
    }

    std::cout << "Done. Generated " << opts.num_lines << " lines to " << opts.output_file << "\n";
    return 0;
  }

} // namespace logengine