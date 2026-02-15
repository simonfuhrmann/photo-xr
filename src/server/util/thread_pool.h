#ifndef SRC_UTIL_THREAD_POOL_H_
#define SRC_UTIL_THREAD_POOL_H_

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace util {

// Thread pool implementation based on C++ STL concurrency primitives.
class ThreadPool {
 public:
  using WorkerFn = std::function<void()>;

  // Creates a thread pool with `num_workers` and starts the workers.
  explicit ThreadPool(int num_workers);

  // Joins the worker pool using JoinPool().
  ~ThreadPool();

  // Queues a work job. The job is executed immediately if workers are
  // available. Jobs are executed in FIFO order.
  void Dispatch(WorkerFn fn);

  // Joins all worker threads. All remaining waiting jobs will be executed,
  // worker threads are destroyed when all jobs are finished.
  void JoinPool();

  // Disallow copy and assign.
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

 private:
  void CreateWorkers(int num_workers);
  void WorkerLoop();

  std::mutex mutex_;
  std::condition_variable queue_cv_;
  std::queue<WorkerFn> work_queue_;
  std::vector<std::thread> workers_;
  bool stop_ = false;
};

// Inline implementation.

inline ThreadPool::ThreadPool(int num_workers) { CreateWorkers(num_workers); }

inline ThreadPool::~ThreadPool() { JoinPool(); }

inline void ThreadPool::Dispatch(WorkerFn fn) {
  assert(!stop_);
  std::unique_lock<std::mutex> lock(mutex_);
  work_queue_.emplace(std::move(fn));
  queue_cv_.notify_one();
}

inline void ThreadPool::JoinPool() {
  if (stop_) return;
  stop_ = true;
  queue_cv_.notify_all();
  for (std::thread& worker : workers_) worker.join();
  std::unique_lock<std::mutex> lock(mutex_);
  std::vector<std::thread>().swap(workers_);
  std::queue<WorkerFn>().swap(work_queue_);
}

inline void ThreadPool::CreateWorkers(int num_workers) {
  assert(num_workers > 0);
  assert(workers_.empty());
  workers_.reserve(num_workers);
  for (int i = 0; i < num_workers; ++i) {
    workers_.emplace_back(&ThreadPool::WorkerLoop, this);
  }
}

inline void ThreadPool::WorkerLoop() {
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_cv_.wait(lock, [&] { return !work_queue_.empty() || stop_; });
    if (stop_ && work_queue_.empty()) break;
    WorkerFn fn = std::move(work_queue_.front());
    work_queue_.pop();
    lock.unlock();
    fn();
  }
}

}  // namespace util

#endif  // SRC_UTIL_THREAD_POOL_H_
