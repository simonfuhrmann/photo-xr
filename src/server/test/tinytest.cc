#include "src/server/test/tinytest.h"

#include <chrono>
#include <cstdint>
#include <cstdio>

#ifndef TINY_TEST_NO_COLORS
#define TT_COLOR_RED "\x1b[1;31m"
#define TT_COLOR_GREEN "\x1b[1;32m"
#define TT_COLOR_RESET "\x1b[0m"
#else  // TINY_TEST_NO_COLORS
#define TT_COLOR_RED ""
#define TT_COLOR_GREEN ""
#define TT_COLOR_RESET ""
#endif  // TINY_TEST_NO_COLORS

namespace {
using Clock = std::chrono::high_resolution_clock;
using Time = Clock::time_point;
using Duration = Clock::duration;
using Millis = std::chrono::milliseconds;
}  // namespace

// Definition of the extern declaration in the header. Must use std::list here,
// otherwise TinyTestRegister() cannot return a stable pointer to TinyTestInfo.
std::list<TinyTestInfo> test_regs = {};

TinyTestInfo* TinyTestRegister(const char* suite, const char* name,
                               TinyTestBase* instance) {
  TinyTestInfo info;
  info.suite_name = suite;
  info.test_name = name;
  info.instance.reset(instance);
  test_regs.push_back(std::move(info));
  return &test_regs.back();
}

int main() {
  printf("[==========] Running %ld tests.\n", test_regs.size());

  int num_total = 0;
  int num_failed = 0;
  int num_passed = 0;
  const Time all_tests_start = Clock::now();
  for (TinyTestInfo& reg : test_regs) {
    printf("%s[ RUN      ]%s %s.%s\n", TT_COLOR_GREEN, TT_COLOR_RESET,
           reg.suite_name, reg.test_name);

    const Time test_start = Clock::now();
    try {
      reg.instance->TestBody();
    } catch (...) {
    }
    const Time test_end = Clock::now();
    const Duration elapsed = test_end - test_start;
    const int64_t ms = std::chrono::duration_cast<Millis>(elapsed).count();

    num_total += 1;
    num_failed += !reg.failures.empty();
    num_passed += reg.failures.empty();

    if (reg.failures.empty()) {
      printf("%s[       OK ]%s %s.%s (%ld ms)\n", TT_COLOR_GREEN,
             TT_COLOR_RESET, reg.suite_name, reg.test_name, ms);
    } else {
      for (const TinyTestFailure& failure : reg.failures) {
        printf("%s[          ]%s Error: %s:%d\n", TT_COLOR_RED, TT_COLOR_RESET,
               failure.file, failure.line);
      }
      printf("%s[  FAILED  ]%s %s.%s (%ld ms)\n", TT_COLOR_RED, TT_COLOR_RESET,
             reg.suite_name, reg.test_name, ms);
    }
  }
  const Time all_tests_end = Clock::now();
  const Duration all_tests_elapsed = all_tests_end - all_tests_start;
  const int64_t all_tests_ms =
      std::chrono::duration_cast<Millis>(all_tests_elapsed).count();

  printf("[==========] %d tests ran (%ld ms total).\n", num_total,
         all_tests_ms);
  if (num_failed == 0) {
    printf("%s[  PASSED  ]%s %d tests.\n", TT_COLOR_GREEN, TT_COLOR_RESET,
           num_passed);
  } else {
    printf("%s[  FAILED  ]%s %d tests failed, %d passed.\n", TT_COLOR_RED,
           TT_COLOR_RESET, num_failed, num_passed);
  }

  // Clean up all registered tests.
  test_regs.clear();
  return num_failed;
}
