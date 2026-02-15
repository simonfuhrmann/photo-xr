#include "src/server/net/http_types.h"

#include <string_view>

#include "src/server/test/tinytest.h"

namespace net {

TEST(HttpTypesTest, StatusCodeConversion) {
  std::string_view str = HttpCodeToString(HttpStatus::CODE_100_CONTINUE);
  EXPECT_EQ(str, "Continue");

  str = HttpCodeToString(HttpStatus::INVALID);
  EXPECT_EQ(str, "Unknown Status");
}

TEST(HttpTypesTest, MethodConversion) {
  std::string_view str = HttpMethodToString(HttpMethod::GET);
  EXPECT_EQ(str, "GET");
  EXPECT_EQ(HttpMethodFromString(str), HttpMethod::GET);

  str = HttpMethodToString(HttpMethod::POST);
  EXPECT_EQ(str, "POST");
  EXPECT_EQ(HttpMethodFromString(str), HttpMethod::POST);
}

TEST(HttpTypesTest, HttpVersionConversion) {
  std::string_view http10 = HttpVersionToString(HttpVersion::HTTP_1_0);
  std::string_view http11 = HttpVersionToString(HttpVersion::HTTP_1_1);
  std::string_view invalid = HttpVersionToString(HttpVersion::INVALID);
  EXPECT_EQ(http10, "HTTP/1.0");
  EXPECT_EQ(http11, "HTTP/1.1");
  EXPECT_EQ(invalid, "INVALID");
  EXPECT_EQ(HttpVersionFromString(http10), HttpVersion::HTTP_1_0);
  EXPECT_EQ(HttpVersionFromString(http11), HttpVersion::HTTP_1_1);
  EXPECT_EQ(HttpVersionFromString(invalid), HttpVersion::INVALID);
}

}  // namespace net
