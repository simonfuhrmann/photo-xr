#include "src/server/util/base64.h"

#include "src/server/test/tinytest.h"
#include "src/server/util/status_or.h"

namespace util {

TEST(Base64, Encode) {
  // Test empty input.
  EXPECT_EQ(Base64Encode(nullptr, 0), "");
  EXPECT_EQ(Base64Encode(""), "");

  // Test examples from Wikipedia.
  EXPECT_EQ(Base64Encode("Man"), "TWFu");
  EXPECT_EQ(Base64Encode("Ma"), "TWE=");
  EXPECT_EQ(Base64Encode("M"), "TQ==");
}

TEST(Base64, Decode) {
  // Test empty input.
  EXPECT_EQ(*Base64Decode(""), std::string());

  // Test examples from Wikipedia.
  EXPECT_EQ(*Base64Decode("TWFu"), std::string("Man"));
  EXPECT_EQ(*Base64Decode("TWE="), std::string("Ma"));
  EXPECT_EQ(*Base64Decode("TQ=="), std::string("M"));
}

TEST(Base64, EncodeDecode) {
  const std::vector<std::string> test_cases = {
      "Hello World!",
      "The quick brown fox jumps over the lazy dog.",
      "This\nis\ta\x01test\x03string.",
  };
  for (const std::string& test : test_cases) {
    EXPECT_EQ(*Base64Decode(Base64Encode(test)), test);
  }
}

TEST(Base64, DecodeError) {
  EXPECT_NOT_OK(Base64Decode("TQ="));   // Invalid length.
  EXPECT_NOT_OK(Base64Decode("T@=="));  // Invalid character '@'.
  EXPECT_NOT_OK(Base64Decode("T=Q="));  // Invalid padding.
  EXPECT_NOT_OK(Base64Decode("===="));  // Invalid padding.
}

}  // namespace util
