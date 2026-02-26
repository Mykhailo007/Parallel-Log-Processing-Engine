#pragma once
#include "thread_pool.h"
#include "aggregator/metrics.h"
#include <string>
#include <vector>

namespace logengine
{

  class Engine
  {
  public:
    explicit Engine(size_t num_threads = 4);

    Metrics process_file(const std::string &file_path);

  private:
    size_t num_threads_;
    static constexpr size_t CHUNK_SIZE = 1024 * 1024; // 1MB chunks

    struct Chunk
    {
      std::string data;
      size_t offset;
    };

    std::vector<Chunk> split_into_chunks(const std::string &file_path);
  };

} // namespace logengine