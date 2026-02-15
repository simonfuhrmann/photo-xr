#include "src/server/net/http_req_target.h"

#include "src/server/test/tinytest.h"

namespace net {

TEST(HttpReqTargetTest, TestWithoutQuery) {
  std::string_view target_str = "/index.html";
  HttpReqTarget target(target_str);
  EXPECT_EQ(target.GetTarget(), target_str);
  EXPECT_EQ(target.GetPath(), "/index.html");
  EXPECT_EQ(target.GetQuery(), "");
  EXPECT_EQ(target.GetParam("a"), "");
}

TEST(HttpReqTargetTest, TestWithQuery) {
  std::string_view target_str = "/index.html?a=123&b=c";
  HttpReqTarget target(target_str);
  EXPECT_EQ(target.GetTarget(), target_str);
  EXPECT_EQ(target.GetPath(), "/index.html");
  EXPECT_EQ(target.GetQuery(), "a=123&b=c");
  EXPECT_EQ(target.GetParam("a"), "123");
  EXPECT_EQ(target.GetParam("b"), "c");
  EXPECT_EQ(target.GetParam("c"), "");
  EXPECT_EQ(target.GetParamInt("a", 0), 123);
  EXPECT_EQ(target.GetParamInt("b", 0), 0);
  EXPECT_EQ(target.GetParamInt("c", 0), 0);
}

}  // namespace net
