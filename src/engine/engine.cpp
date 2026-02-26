#include "engine.h"
#include "parser/parser.h"
#include "aggregator/aggregator.h"
#include "utils/file_reader.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <future>

namespace logengine
{

  Engine::Engine(size_t num_threads) : num_threads_(num_threads) {}

  std::vector<Engine::Chunk> Engine::split_into_chunks(const std::string &file_path)
  {
    std::vector<Chunk> chunks;
    std::ifstream file(file_path, std::ios::binary);

    if (!file)
    {
      throw std::runtime_error("Cannot open file: " + file_path);
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t offset = 0;
    while (offset < file_size)
    {
      file.seekg(offset);

      std::string buffer(CHUNK_SIZE, '\0');
      file.read(&buffer[0], CHUNK_SIZE);
      size_t bytes_read = file.gcount();
      buffer.resize(bytes_read);

      // Don't split mid-line: read until newline
      if (offset + bytes_read < file_size && !buffer.empty() && buffer.back() != '\n')
      {
        std::string extra;
        std::getline(file, extra);
        buffer += extra;
        if (!extra.empty() && extra.back() != '\n')
        {
          buffer += '\n';
        }
      }

      chunks.push_back({std::move(buffer), offset});
      offset += bytes_read;
    }

    return chunks;
  }

  Metrics Engine::process_file(const std::string &file_path)
  {
    auto chunks = split_into_chunks(file_path);

    ThreadPool pool(num_threads_);
    std::vector<std::future<Aggregator>> futures;

    // Process each chunk in parallel
    for (auto &chunk : chunks)
    {
      futures.push_back(pool.enqueue([data = std::move(chunk.data)]()
                                     {
            Aggregator agg;
            Parser parser;
            
            size_t start = 0;
            size_t pos = 0;
            
            while (pos < data.size()) {
                if (data[pos] == '\n') {
                    if (pos > start) {
                        std::string_view line(data.data() + start, pos - start);
                        auto entry = parser.parse_line(line);
                        if (entry.has_value()) {
                            agg.add_entry(entry.value());
                        }
                    }
                    start = pos + 1;
                }
                pos++;
            }
            
            // Handle last line if no trailing newline
            if (start < data.size()) {
                std::string_view line(data.data() + start, data.size() - start);
                auto entry = parser.parse_line(line);
                if (entry.has_value()) {
                    agg.add_entry(entry.value());
                }
            }
            
            return agg; }));
    }

    // Merge results from all threads
    Aggregator final_agg;
    for (auto &fut : futures)
    {
      final_agg.merge(fut.get());
    }

    return final_agg.get_metrics();
  }

} // namespace logengine