#include "src/server/net/http_req_target.h"

#include "src/server/test/tinytest.h"

namespace net {

TEST(HttpReqTargetTest, TestInvalidPath) {
  HttpReqTarget target;
  // Leading slash required.
  EXPECT_FALSE(target.Parse("").ok());
  EXPECT_FALSE(target.Parse("index.html").ok());
  // Encoded '/' not allowed in path.
  EXPECT_FALSE(target.Parse("/data%2findex.html").ok());
  // Path outside of root directory not allowed.
  EXPECT_FALSE(target.Parse("/../index.html").ok());
}

TEST(HttpReqTargetTest, TestWithoutQuery) {
  std::string_view target_str = "/index.html";
  HttpReqTarget target;
  ASSERT_OK(target.Parse(target_str));
  EXPECT_EQ(target.GetTarget(), target_str);
  EXPECT_EQ(target.GetPath(), "/index.html");
  EXPECT_EQ(target.GetNormalizedPath(), "/index.html");
  EXPECT_EQ(target.GetQuery(), "");
  EXPECT_EQ(target.GetParam("a"), "");
}

TEST(HttpReqTargetTest, TestWithQuery) {
  std::string_view target_str = "/index.html?a=123&b=c";
  HttpReqTarget target;
  ASSERT_OK(target.Parse(target_str));
  EXPECT_EQ(target.GetTarget(), target_str);
  EXPECT_EQ(target.GetPath(), "/index.html");
  EXPECT_EQ(target.GetNormalizedPath(), "/index.html");
  EXPECT_EQ(target.GetQuery(), "a=123&b=c");
  EXPECT_EQ(target.GetParam("a"), "123");
  EXPECT_EQ(target.GetParam("b"), "c");
  EXPECT_EQ(target.GetParam("c"), "");
  EXPECT_EQ(target.GetParamInt("a", 0), 123);
  EXPECT_EQ(target.GetParamInt("b", 0), 0);
  EXPECT_EQ(target.GetParamInt("c", 0), 0);
}

TEST(HttpReqTargetTest, TestWithEncodedChars) {
  std::string_view target_str = "/data/foo%20bar?user=alice%20bob&debug=1";
  HttpReqTarget target;
  ASSERT_OK(target.Parse(target_str));
  EXPECT_EQ(target.GetTarget(), target_str);
  EXPECT_EQ(target.GetPath(), "/data/foo%20bar");
  EXPECT_EQ(target.GetNormalizedPath(), "/data/foo bar");
  EXPECT_EQ(target.GetQuery(), "user=alice%20bob&debug=1");

  const HttpReqTarget::PathComponents& paths = target.GetPathComponents();
  EXPECT_EQ(paths.size(), 2);
  EXPECT_EQ(paths[0], "data");
  EXPECT_EQ(paths[1], "foo bar");
  const HttpReqTarget::QueryParamsMap& params = target.GetQueryParams();
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params.at("user"), "alice bob");
  EXPECT_EQ(params.at("debug"), "1");
  EXPECT_EQ(target.GetParam("user"), "alice bob");
  EXPECT_EQ(target.GetParam("debug"), "1");
}


}  // namespace net
