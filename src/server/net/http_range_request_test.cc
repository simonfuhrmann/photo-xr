#include "src/server/net/http_range_request.h"

#include "src/server/test/tinytest.h"

namespace net {

TEST(HttpRangeRequest, ParseRangeRequest) {
  util::StatusOr<RangeRequest> result;

  // Full-range.
  result = ParseRangeRequest("bytes=1000-2000");
  ASSERT_OK(result);
  EXPECT_EQ(result->is_range_request, true);
  EXPECT_EQ(result->start, 1000);
  EXPECT_EQ(result->end, 2000);

  // Open-range.
  result = ParseRangeRequest("bytes=1000-");
  ASSERT_OK(result);
  EXPECT_EQ(result->is_range_request, true);
  EXPECT_EQ(result->start, 1000);
  EXPECT_EQ(result->end, -1);

  // Suffix-length.
  result = ParseRangeRequest("bytes=-500");
  ASSERT_OK(result);
  EXPECT_EQ(result->is_range_request, true);
  EXPECT_EQ(result->start, -1);
  EXPECT_EQ(result->end, 500);

  // Invalid requests.
  EXPECT_FALSE(ParseRangeRequest("").ok());
  EXPECT_FALSE(ParseRangeRequest("bytes=2000-1000").ok());    // Flipped range
  EXPECT_FALSE(ParseRangeRequest("foos=1000-2000").ok());     // Needs "bytes"
  EXPECT_FALSE(ParseRangeRequest("bytes=").ok());             // No range
  EXPECT_FALSE(ParseRangeRequest("bytes=-").ok());            // Invalid range
  EXPECT_FALSE(ParseRangeRequest("bytes=1000").ok());         // No hyphen
  EXPECT_FALSE(ParseRangeRequest("bytes=100-200-300").ok());  // Invalid range
  EXPECT_FALSE(ParseRangeRequest("bytes=abc-def").ok());      // Not integer
}

TEST(HttpRangeRequest, GenerateRangeResponse) {
  std::string test_string = "0123456789";
  util::StatusOr<RangeResponse> response;

  // Not a range request.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = false;
    response = GenerateRangeResponse(request, input);
    ASSERT_FALSE(response.ok());
  }

  // Invalid range request.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = true;
    request.start = -1;
    request.end = -1;
    response = GenerateRangeResponse(request, input);
    ASSERT_FALSE(response.ok());
  }

  // Satisfiable range.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = true;
    request.start = 1;
    request.end = 3;
    response = GenerateRangeResponse(request, input);
    ASSERT_OK(response);
    EXPECT_EQ(response->start, 1);
    EXPECT_EQ(response->end, 3);
    EXPECT_EQ(response->total, 10);
    EXPECT_EQ(input.tellg(), 1);
  }

  // Open range.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = true;
    request.start = 7;
    request.end = -1;
    response = GenerateRangeResponse(request, input);
    ASSERT_OK(response);
    EXPECT_EQ(response->start, 7);
    EXPECT_EQ(response->end, 9);
    EXPECT_EQ(response->total, 10);
    EXPECT_EQ(input.tellg(), 7);
  }

  // Suffix length.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = true;
    request.start = -1;
    request.end = 4;
    response = GenerateRangeResponse(request, input);
    ASSERT_OK(response);
    EXPECT_EQ(response->start, 6);
    EXPECT_EQ(response->end, 9);
    EXPECT_EQ(response->total, 10);
    EXPECT_EQ(input.tellg(), 6);
  }

  // File out of bounds is not satisfiable.
  {
    std::istringstream input(test_string);
    RangeRequest request;
    request.is_range_request = true;
    request.start = 5;
    request.end = 10;
    response = GenerateRangeResponse(request, input);
    ASSERT_OK(response);
    EXPECT_EQ(response->start, -1);
    EXPECT_EQ(response->end, -1);
    EXPECT_EQ(response->total, 10);
    EXPECT_EQ(input.tellg(), 0);
  }

  // Zero byte input file cannot be satisfied.
  {
    std::istringstream input("");
    RangeRequest request;
    request.is_range_request = true;
    request.start = 0;
    request.end = 0;
    response = GenerateRangeResponse(request, input);
    ASSERT_OK(response);
    EXPECT_EQ(response->start, -1);
    EXPECT_EQ(response->end, -1);
    EXPECT_EQ(response->total, 0);
    EXPECT_EQ(input.tellg(), 0);
  }
}

TEST(HttpRangeRequest, GetContentRangeHeader) {
  // Satisfiable range.
  {
    RangeResponse response;
    response.start = 1000;
    response.end = 2000;
    response.total = 3412;
    EXPECT_EQ(GetContentRangeHeader(response), "bytes 1000-2000/3412");
  }

  // Unsatisfiable range with known total.
  {
    RangeResponse response;
    response.start = -1;
    response.end = -1;
    response.total = 3412;
    EXPECT_EQ(GetContentRangeHeader(response), "bytes */3412");
  }

  // Unsatisfiable range with unknown total.
  {
    RangeResponse response;
    response.start = -1;
    response.end = -1;
    response.total = -1;
    EXPECT_EQ(GetContentRangeHeader(response), "bytes */*");
  }
}

}  // namespace net
