#include "src/server/util/status.h"

#include <cerrno>
#include <cmath>

#include "src/server/test/tinytest.h"

namespace util {

Status TestReturnIfError() {
  RETURN_IF_ERROR(OkStatus());
  RETURN_IF_ERROR(InternalError("internal"));
  return OkStatus();
}

TEST(StatusTest, BasicTests) {
  const Status ok1 = OkStatus();
  const Status ok2;
  const Status ok3(StatusCode::OK, "");
  const Status ok4(StatusCode::OK, "ignored");
  EXPECT_EQ(ok1, ok2);
  EXPECT_EQ(ok1, ok3);
  EXPECT_EQ(ok1, ok4);
  EXPECT_OK(ok1);
  EXPECT_EQ(ok1.code(), StatusCode::OK);
  EXPECT_EQ(ok1.message(), "");
  EXPECT_EQ(ok4.message(), "");

  const Status internal1 = InternalError("test");
  const Status internal2(StatusCode::INTERNAL, "test");
  EXPECT_EQ(internal1, internal2);
  EXPECT_NE(ok1, internal1);
  EXPECT_NOT_OK(internal1);
  EXPECT_EQ(internal1.code(), StatusCode::INTERNAL);
  EXPECT_EQ(internal1.message(), "test");

  const Status prefixed = internal1.WithPrefix("prefix");
  EXPECT_EQ(prefixed.message(), "prefix: test");
  EXPECT_EQ(internal1.message(), "test");

  const Status suffixed = internal1.WithSuffix("suffix");
  EXPECT_EQ(suffixed.message(), "test: suffix");
  EXPECT_EQ(internal1.message(), "test");
}

TEST(StatusTest, ReturnIfError) {
  EXPECT_EQ(TestReturnIfError(), InternalError("internal"));
}

TEST(StatusTest, StatusFromErrno) {
  const Status status = StatusFromErrno(EINVAL);
  EXPECT_EQ(status.code(), StatusCode::INVALID_ARGUMENT);
}

TEST(StatusTest, StatusFromErrnoExample) {
  errno = 0;
  std::sqrt(-1.0);  // Generaates EDOM.
  const Status status = StatusFromErrno(errno);
  EXPECT_NOT_OK(status);
  EXPECT_EQ(status.code(), StatusCode::INVALID_ARGUMENT);
  EXPECT_EQ(status.message(), "Numerical argument out of domain");
}

TEST(StatusTest, ExitProgramIfStatusError) {
  // Test the program does not die with Ok status.
  util::Status status;
  ExitProgramIfStatusError(status);
  EXIT_IF_ERROR(status);
  EXPECT_TRUE(true);
}

}  // namespace util
