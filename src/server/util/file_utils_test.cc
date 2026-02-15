#include "src/server/util/file_utils.h"

#include <iostream>  // rm

#include "src/server/test/tinytest.h"

namespace util {
namespace {

TEST(FileUtilsTest, GetAlignedSize) {
  EXPECT_EQ(GetAlignedSize(0), 0);
  EXPECT_EQ(GetAlignedSize(1), 4);
  EXPECT_EQ(GetAlignedSize(2), 4);
  EXPECT_EQ(GetAlignedSize(3), 4);
  EXPECT_EQ(GetAlignedSize(4), 4);
  EXPECT_EQ(GetAlignedSize(5), 8);
}

TEST(FileUtilsTest, GetFileExtension) {
  EXPECT_EQ(GetFileExtension("/file.txt"), "txt");
  EXPECT_EQ(GetFileExtension("/file.tar.gz"), "gz");
  EXPECT_EQ(GetFileExtension("/file"), "");
  EXPECT_EQ(GetFileExtension(""), "");
  EXPECT_EQ(GetFileExtension("/dir.ext/file.txt"), "txt");
  EXPECT_EQ(GetFileExtension("/dir.ext/file"), "");
}

TEST(FileUtilsTest, GetNormalizedPath) {
  EXPECT_EQ(GetNormalizedPath(""), "");
  EXPECT_EQ(GetNormalizedPath("."), ".");
  EXPECT_EQ(GetNormalizedPath("./"), ".");  // strange
  EXPECT_EQ(GetNormalizedPath("/dir/"), "/dir/");
  EXPECT_EQ(GetNormalizedPath("/dir/../file"), "/file");
  EXPECT_EQ(GetNormalizedPath("/../file"), "/file");
  EXPECT_EQ(GetNormalizedPath("/./file"), "/file");
}

}  // namespace
}  // namespace util
