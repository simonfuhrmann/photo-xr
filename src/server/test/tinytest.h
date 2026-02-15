// An incredibly tiny and simple C++ unit testing library.
//
// Example usage:
//
//   #include "mylib.h"
//   #include "tinytest.h"
//
//   TEST(MyLib, GetNumbers) {
//     const std::vector<int> numbers = GetNumbers();
//     ASSERT_EQ(numbers.size(), 1);
//     EXPECT_EQ(numbers[0], 32);
//   }
//
// There are two kinds of test macros, EXPECT_* and ASSERT_*. The former
// generate non-fatal failures which do not abort the test. The latter exit
// the test on failure (by throwing an exception). EXPECT is preferred when it
// makes sense to continue the test. ASSERT is, however, required when it is
// unsave to continue the test (e.g., see the above example).
//
//   EXPECT_TRUE(x) - Succeeds if x == true.
//   EXPECT_FALSE(x) - Succeeds if x == false.
//   EXPECT_EQ(a, b) - Succeeds if a == b.
//   EXPECT_NE(a, b) - Succeeds if a != b.

#ifndef SRC_TEST_TINYTEST_H_
#define SRC_TEST_TINYTEST_H_

#include <list>
#include <memory>
#include <stdexcept>

// Base class for all tests.
class TinyTestBase {
 public:
  virtual void TestBody() = 0;
};

// Record of a test failure.
struct TinyTestFailure {
  const char* file;
  int line;
};

// Information for each registered test.
struct TinyTestInfo {
  const char* suite_name;
  const char* test_name;
  std::unique_ptr<TinyTestBase> instance;
  std::list<TinyTestFailure> failures;
};

// Static variable with all registered tests.
extern std::list<TinyTestInfo> test_regs;

// Registers a test.
TinyTestInfo* TinyTestRegister(const char* suite, const char* name,
                               TinyTestBase* instance);

// Internal class name representing a single test.
#define TEST_CLASS_NAME(suite, name) suite##_##name##_Test

// Helper to register a new single test.
#define TEST_REGISTER(suite, name) \
  TinyTestRegister(#suite, #name, new TEST_CLASS_NAME(suite, name)());

// Public-facing macro to define a new test.
#define TEST(suite, name)                                    \
  class TEST_CLASS_NAME(suite, name) : public TinyTestBase { \
   public:                                                   \
    void TestBody() override;                                \
    static TinyTestInfo* test_info;                          \
  };                                                         \
  TinyTestInfo* TEST_CLASS_NAME(suite, name)::test_info =    \
      TEST_REGISTER(suite, name);                            \
  void TEST_CLASS_NAME(suite, name)::TestBody()

// Internal helper macros.
#define TEST_MAKE_ERROR() {.file = __FILE__, .line = __LINE__}
#define TEST_ADD_ERROR() test_info->failures.push_back(TEST_MAKE_ERROR())
#define TEST_THROW_EXCEPTION() throw std::runtime_error("")

#define TEST_COMPARE_OP(a, b, op, throw) \
  do {                                   \
    if (!((a)op(b))) {                   \
      TEST_ADD_ERROR();                  \
      if (throw) TEST_THROW_EXCEPTION(); \
    }                                    \
  } while (false)

#define TEST_COMPARE_EPS(a, b, eps, throw) \
  do {                                     \
    if (a < b - eps || a > b + eps) {      \
      TEST_ADD_ERROR();                    \
      if (throw) TEST_THROW_EXCEPTION();   \
    }                                      \
  } while (false)

#define TEST_COMPARE_EXPECT(a, b, op) TEST_COMPARE_OP(a, b, op, false)
#define TEST_COMPARE_ASSERT(a, b, op) TEST_COMPARE_OP(a, b, op, true)
#define TEST_COMPARE_NEAR_EXPECT(a, b, eps) TEST_COMPARE_EPS(a, b, eps, false)
#define TEST_COMPARE_NEAR_ASSERT(a, b, eps) TEST_COMPARE_EPS(a, b, eps, true)

// Public-facing macros to expect/assert.
#define EXPECT_TRUE(a) TEST_COMPARE_EXPECT(a, true, ==)
#define EXPECT_FALSE(a) TEST_COMPARE_EXPECT(a, false, ==)
#define EXPECT_EQ(a, b) TEST_COMPARE_EXPECT(a, b, ==)
#define EXPECT_NE(a, b) TEST_COMPARE_EXPECT(a, b, !=)
#define EXPECT_GT(a, b) TEST_COMPARE_EXPECT(a, b, >)
#define EXPECT_GE(a, b) TEST_COMPARE_EXPECT(a, b, >=)
#define EXPECT_LT(a, b) TEST_COMPARE_EXPECT(a, b, <)
#define EXPECT_LE(a, b) TEST_COMPARE_EXPECT(a, b, <=)
#define EXPECT_NEAR(a, b, eps) TEST_COMPARE_NEAR_EXPECT(a, b, eps)
#define EXPECT_OK(a) EXPECT_TRUE((a).ok())
#define EXPECT_NOT_OK(a) EXPECT_FALSE((a).ok())

#define ASSERT_TRUE(a) TEST_COMPARE_ASSERT(a, true, ==)
#define ASSERT_FALSE(a) TEST_COMPARE_ASSERT(a, false, ==)
#define ASSERT_EQ(a, b) TEST_COMPARE_ASSERT(a, b, ==)
#define ASSERT_NE(a, b) TEST_COMPARE_ASSERT(a, b, !=)
#define ASSERT_GT(a, b) TEST_COMPARE_ASSERT(a, b, >)
#define ASSERT_GE(a, b) TEST_COMPARE_ASSERT(a, b, >=)
#define ASSERT_LT(a, b) TEST_COMPARE_ASSERT(a, b, <)
#define ASSERT_LE(a, b) TEST_COMPARE_ASSERT(a, b, <=)
#define ASSERT_NEAR(a, b, eps) TEST_COMPARE_NEAR_ASSERT(a, b, eps)
#define ASSERT_OK(a) ASSERT_TRUE((a).ok())
#define ASSERT_NOT_OK(a) ASSERT_FALSE((a).ok())

#endif  // SRC_TEST_TINYTEST_H_
