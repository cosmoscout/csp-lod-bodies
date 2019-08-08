////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_THREADPOOL_HPP
#define CSP_LOD_BODIES_THREADPOOL_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <thread>
#include <vector>

namespace csp::lodbodies {

/// This is based on https://github.com/progschj/ThreadPool
class ThreadPool {
 public:
  /// Creates a new ThreadPool with the specified amount of threads.
  ThreadPool(size_t threadCount);
  virtual ~ThreadPool();

  /// Adds a new work item to the pool.
  template <class F, class... Args>
  auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mMutex);

      if (mStop) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      mTasks.emplace([task]() { (*task)(); });
    }
    mCondition.notify_one();
    return res;
  }

  /// Returns the amount of tasks that await execution.
  int PendingTaskCount() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mTasks.size();
  }

  /// Returns the number of tasks that currently are being executed.
  int RunningTaskCount() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mRunningTasks;
  }

 private:
  std::vector<std::thread>          mWorkers;
  std::stack<std::function<void()>> mTasks;
  std::mutex                        mMutex;
  std::condition_variable           mCondition;
  bool                              mStop;
  int                               mRunningTasks = 0;
};

} // namespace csp::lodbodies

#endif // CSP_LOD_BODIES_THREADPOOL_HPP
