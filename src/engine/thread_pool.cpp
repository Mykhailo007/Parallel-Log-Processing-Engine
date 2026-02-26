#include "thread_pool.h"

namespace logengine
{

  ThreadPool::ThreadPool(size_t num_threads)
  {
    for (size_t i = 0; i < num_threads; i++)
    {
      workers_.emplace_back([this]
                            {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] { 
                        return stop_ || !tasks_.empty(); 
                    });
                    
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    
                    if (!tasks_.empty()) {
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                }
                
                if (task) {
                    task();
                    active_tasks_--;
                    wait_condition_.notify_all();
                }
            } });
    }
  }

  ThreadPool::~ThreadPool()
  {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      stop_ = true;
    }
    condition_.notify_all();
    for (auto &worker : workers_)
    {
      if (worker.joinable())
      {
        worker.join();
      }
    }
  }

  void ThreadPool::wait()
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    wait_condition_.wait(lock, [this]
                         { return tasks_.empty() && active_tasks_ == 0; });
  }

} // namespace logengine