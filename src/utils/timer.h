#pragma once
#include <chrono>

namespace logengine
{

  class Timer
  {
  public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const
    {
      auto end = std::chrono::high_resolution_clock::now();
      return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    void reset()
    {
      start_ = std::chrono::high_resolution_clock::now();
    }

  private:
    std::chrono::high_resolution_clock::time_point start_;
  };

} // namespace logengine