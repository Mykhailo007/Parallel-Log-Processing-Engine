#pragma once
#include <string>
#include <fstream>
#include <stdexcept>

namespace logengine
{

  class FileReader
  {
  public:
    static std::string read_file(const std::string &path)
    {
      std::ifstream file(path, std::ios::binary);
      if (!file)
      {
        throw std::runtime_error("Cannot open file: " + path);
      }

      file.seekg(0, std::ios::end);
      size_t size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::string buffer(size, '\0');
      file.read(&buffer[0], size);

      return buffer;
    }
  };

} // namespace logengine