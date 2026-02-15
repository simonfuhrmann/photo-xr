#include "src/server/util/string_utils.h"

#include <string>

#include "src/server/test/tinytest.h"

namespace util {

TEST(StringUtils, StrSplit) {
  std::vector<std::string> split;

  split = util::StrSplit("Hello World");
  ASSERT_EQ(split.size(), 2);
  EXPECT_EQ(split[0], "Hello");
  EXPECT_EQ(split[1], "World");

  split = util::StrSplit("  Hello  World  ");
  ASSERT_EQ(split.size(), 2);
  EXPECT_EQ(split[0], "Hello");
  EXPECT_EQ(split[1], "World");

  split = util::StrSplit("HelloXWorld", 'X');
  ASSERT_EQ(split.size(), 2);
  EXPECT_EQ(split[0], "Hello");
  EXPECT_EQ(split[1], "World");

  split = util::StrSplit("  Hello  World ", ' ', /*keep_empty=*/true);
  ASSERT_EQ(split.size(), 6);
  EXPECT_EQ(split[0], "");
  EXPECT_EQ(split[1], "");
  EXPECT_EQ(split[2], "Hello");
  EXPECT_EQ(split[3], "");
  EXPECT_EQ(split[4], "World");
  EXPECT_EQ(split[5], "");
}

TEST(StringUtils, StrCat) {
  std::string_view str_view = "StringView";
  std::string str = "String";
  EXPECT_EQ(StrCat(), "");
  EXPECT_EQ(StrCat(123), "123");
  EXPECT_EQ(StrCat("Hello", "World"), "HelloWorld");
  EXPECT_EQ(StrCat("Hello", 123), "Hello123");
  EXPECT_EQ(StrCat("Hello", 123.1), "Hello123.1");
  EXPECT_EQ(StrCat("Hello", str_view), "HelloStringView");
  EXPECT_EQ(StrCat("Hello", str), "HelloString");
}

TEST(StringUtils, StrToInt) {
  int value = -1;
  EXPECT_TRUE(StrToInt("123", &value));
  EXPECT_EQ(value, 123);

  EXPECT_FALSE(StrToInt("", &value));
  EXPECT_FALSE(StrToInt("a", &value));
  EXPECT_FALSE(StrToInt("a123", &value));
  EXPECT_FALSE(StrToInt("123a", &value));
}

TEST(StringUtils, ToLowercase) {
  {
    std::string str;
    ToLowercase(str);
    EXPECT_EQ(str, "");
  }

  {
    std::string str("To Lowercase");
    ToLowercase(str);
    EXPECT_EQ(str, "to lowercase");
  }
}

TEST(StringUtils, TrimNewlines) {
  EXPECT_EQ(TrimNewlines(""), "");
  EXPECT_EQ(TrimNewlines("Test"), "Test");
  EXPECT_EQ(TrimNewlines("Test\r"), "Test");
  EXPECT_EQ(TrimNewlines("Test\r\n"), "Test");
  EXPECT_EQ(TrimNewlines("Test\r\n\r\n\n"), "Test");
  EXPECT_EQ(TrimNewlines("\r\n"), "");
}

TEST(StringUtils, TrimWhitespaces) {
  EXPECT_EQ(TrimWhitespaces(""), "");
  EXPECT_EQ(TrimWhitespaces("Test"), "Test");
  EXPECT_EQ(TrimWhitespaces("Test  "), "Test");
  EXPECT_EQ(TrimWhitespaces("   Test "), "Test");
}

TEST(StringUtils, TrimString) {
  EXPECT_EQ(TrimString(""), "");
  EXPECT_EQ(TrimString("Test"), "Test");
  EXPECT_EQ(TrimString("Test  "), "Test");
  EXPECT_EQ(TrimString("   Test "), "Test");
  EXPECT_EQ(TrimString("\n\r   Test \r\n"), "Test");
}

TEST(StringUtils, HasPrefix) {
  EXPECT_TRUE(HasPrefix("", ""));
  EXPECT_TRUE(HasPrefix("prefix", "prefix"));
  EXPECT_TRUE(HasPrefix("prefix", "pre"));
  EXPECT_TRUE(HasPrefix("prefix", ""));

  EXPECT_FALSE(HasPrefix("", "prefix"));
  EXPECT_FALSE(HasPrefix("pre", "prefix"));
}

}  // namespace util
