#include "src/server/test/tinytest.h"

#define PI 3.14159265358979323846

namespace tt {
namespace {

TEST(TinyTestTest, Expect) {
  EXPECT_TRUE(true);
  EXPECT_TRUE(1);
  EXPECT_TRUE(1.0);

  EXPECT_FALSE(false);
  EXPECT_FALSE(0);
  EXPECT_FALSE(0.0);

  EXPECT_EQ(true, true);
  EXPECT_EQ(false, false);
  EXPECT_EQ(1, 1);
  EXPECT_EQ(std::string("Hello"), std::string("Hello"));

  EXPECT_NE(true, false);
  EXPECT_NE(1, 2);
  EXPECT_NE(std::string("Hello"), std::string("World"));

  EXPECT_GT(true, false);
  EXPECT_GT(2, 1);
  EXPECT_GT(std::string("B"), std::string("A"));

  EXPECT_GE(true, false);
  EXPECT_GE(2, 1);
  EXPECT_GE(std::string("B"), std::string("A"));
  EXPECT_GE(true, true);
  EXPECT_GE(2, 2);
  EXPECT_GE(std::string("A"), std::string("A"));

  EXPECT_LT(false, true);
  EXPECT_LT(1, 2);
  EXPECT_LT(std::string("A"), std::string("B"));

  EXPECT_LE(false, true);
  EXPECT_LE(1, 2);
  EXPECT_LE(std::string("A"), std::string("B"));
  EXPECT_LE(false, false);
  EXPECT_LE(1, 1);
  EXPECT_LE(std::string("A"), std::string("A"));
}

TEST(TinyTestTest, Assert) {
  ASSERT_TRUE(true);
  ASSERT_TRUE(1);
  ASSERT_TRUE(1.0);

  ASSERT_FALSE(false);
  ASSERT_FALSE(0);
  ASSERT_FALSE(0.0);

  ASSERT_EQ(true, true);
  ASSERT_EQ(false, false);
  ASSERT_EQ(1, 1);
  ASSERT_EQ(std::string("Hello"), std::string("Hello"));

  ASSERT_NE(true, false);
  ASSERT_NE(1, 2);
  ASSERT_NE(std::string("Hello"), std::string("World"));

  ASSERT_GT(true, false);
  ASSERT_GT(2, 1);
  ASSERT_GT(std::string("B"), std::string("A"));

  ASSERT_GE(true, false);
  ASSERT_GE(2, 1);
  ASSERT_GE(std::string("B"), std::string("A"));
  ASSERT_GE(true, true);
  ASSERT_GE(2, 2);
  ASSERT_GE(std::string("A"), std::string("A"));

  ASSERT_LT(false, true);
  ASSERT_LT(1, 2);
  ASSERT_LT(std::string("A"), std::string("B"));

  ASSERT_LE(false, true);
  ASSERT_LE(1, 2);
  ASSERT_LE(std::string("A"), std::string("B"));
  ASSERT_LE(false, false);
  ASSERT_LE(1, 1);
  ASSERT_LE(std::string("A"), std::string("A"));
}

TEST(TinyTestTest, ExpectNear) {
  EXPECT_NEAR(1.0, 1.0, 0.0);
  EXPECT_NEAR(1.0, 1.0, 100.0);
  EXPECT_NEAR(1.0, 2.0, 1.0);
  EXPECT_NEAR(2.0, 1.0, 1.0);
  EXPECT_NEAR(PI, 3.1415, 1e-4);

  EXPECT_NEAR(1, 1, 0);
  EXPECT_NEAR(1, 2, 1);
  EXPECT_NEAR(2, 1, 1);
}

TEST(TinyTestTest, AssertNear) {
  ASSERT_NEAR(1.0, 1.0, 0.0);
  ASSERT_NEAR(1.0, 1.0, 100.0);
  ASSERT_NEAR(1.0, 2.0, 1.0);
  ASSERT_NEAR(2.0, 1.0, 1.0);
  ASSERT_NEAR(PI, 3.1415, 1e-4);

  ASSERT_NEAR(1, 1, 0);
  ASSERT_NEAR(1, 2, 1);
  ASSERT_NEAR(2, 1, 1);
}

}  // namespace
}  // namespace tt
