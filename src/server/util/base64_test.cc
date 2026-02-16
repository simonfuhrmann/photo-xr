#include "src/server/util/base64.h"

#include "src/server/test/tinytest.h"
#include "src/server/util/status_or.h"

namespace util {
namespace {

const uint8_t* Cast(const char* str) {
  return reinterpret_cast<const uint8_t*>(str);
}

bool CompareResult(const StatusOr<std::vector<uint8_t>>& decoded,
                   const std::string& expected) {
  if (!decoded.ok()) return false;
  const std::string_view decoded_view(
      reinterpret_cast<const char*>(decoded->data()), decoded->size());
  return decoded_view == expected;
}

}  // namespace

TEST(Base64, Encode) {
  // Test empty input.
  EXPECT_EQ(Base64Encode(nullptr, 0), "");
  EXPECT_EQ(Base64Encode(Cast(""), 0), "");

  // Test examples from Wikipedia.
  EXPECT_EQ(Base64Encode(Cast("Man"), 3), "TWFu");
  EXPECT_EQ(Base64Encode(Cast("Ma"), 2), "TWE=");
  EXPECT_EQ(Base64Encode(Cast("M"), 1), "TQ==");
}

TEST(Base64, Decode) {
  // Test empty input.
  EXPECT_TRUE(CompareResult(Base64Decode(""), ""));

  // Test examples from Wikipedia.
  EXPECT_TRUE(CompareResult(Base64Decode("TWFu"), "Man"));
  EXPECT_TRUE(CompareResult(Base64Decode("TWE="), "Ma"));
  EXPECT_TRUE(CompareResult(Base64Decode("TQ=="), "M"));
}

TEST(Base64, EncodeDecode) {
  const std::vector<std::string> test_cases = {
      "Hello World!",
      "The quick brown fox jumps over the lazy dog.",
      "This\nis\ta\x01test\x03string.",
  };
  for (const std::string& test : test_cases) {
    const std::string encoded = Base64Encode(Cast(test.data()), test.size());
    EXPECT_TRUE(CompareResult(Base64Decode(encoded), test));
  }
}

TEST(Base64, DecodeError) {
  EXPECT_NOT_OK(Base64Decode("TQ="));   // Invalid length.
  EXPECT_NOT_OK(Base64Decode("T@=="));  // Invalid character '@'.
  EXPECT_NOT_OK(Base64Decode("T=Q="));  // Invalid padding.
  EXPECT_NOT_OK(Base64Decode("===="));  // Invalid padding.
}

}  // namespace util
