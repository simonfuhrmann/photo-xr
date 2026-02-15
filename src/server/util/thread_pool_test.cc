#include "src/server/util/thread_pool.h"

#include <atomic>

#include "src/server/test/tinytest.h"

namespace util {

TEST(ThreadPoolTest, ConstructAndJoinNoWork) {
  ThreadPool pool(8);
  pool.JoinPool();
}

TEST(ThreadPoolTest, DispatchAndJoin) {
  std::atomic_int counter = 0;
  ThreadPool pool(8);
  for (int i = 0; i < 100; ++i) {
    pool.Dispatch([&]() { counter++; });
  }
  pool.JoinPool();
  EXPECT_EQ(counter, 100);
}

TEST(ThreadPoolTest, ConcurrentDispatch) {
  ThreadPool pool(8);
  std::atomic_int counter = 0;

  std::vector<std::thread> dispatchers;
  for (int i = 0; i < 8; ++i) {
    dispatchers.emplace_back([&] {
      for (int j = 0; j < 100; ++j) {
        pool.Dispatch([&] { counter++; });
      }
    });
  }
  for (auto& d : dispatchers) d.join();
  pool.JoinPool();
  EXPECT_EQ(counter, 800);
}

}  // namespace util
