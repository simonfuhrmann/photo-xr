#include "src/server/net/url_encoding.h"

#include "src/server/test/tinytest.h"

namespace net {

TEST(UrlEncodingTest, TestUrlDecode) {
  UrlDecodeOptions options;
  options.allow_encoded_slash = true;
  options.allow_encoded_null = true;
  EXPECT_EQ(UrlDecode(options, "foo%20bar").value(), "foo bar");
  EXPECT_EQ(UrlDecode(options, "foo%3Dbar").value(), "foo=bar");
  EXPECT_EQ(UrlDecode(options, "foo%2Fbar").value(), "foo/bar");
  EXPECT_EQ(UrlDecode(options, "foo%00bar").value(),
            std::string_view("foo\0bar", 7));

  // Forbidden characters.
  options.allow_encoded_slash = false;
  options.allow_encoded_null = false;
  EXPECT_FALSE(UrlDecode(options, "foo%2Fbar").ok());
  EXPECT_FALSE(UrlDecode(options, "foo%00bar").ok());

  // Invalid percent encoding.
  EXPECT_FALSE(UrlDecode(options, "invalid%").ok());
  EXPECT_FALSE(UrlDecode(options, "invalid%1").ok());
}

}  // namespace net
