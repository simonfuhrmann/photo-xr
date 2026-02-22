#ifndef SRC_SERVER_NET_HTTP_RANGE_REQUEST_H_
#define SRC_SERVER_NET_HTTP_RANGE_REQUEST_H_

#include <cstdint>

#include "src/server/net/http_request.h"
#include "src/server/util/status_or.h"

namespace net {

// The client may send a range request via the "range" header, for example:
//
//   Range: bytes=1000-2000     // full-range, start and end range.
//   Range: bytes=1000-         // open-range, start range until end of file.
//   Range: bytes=-500          // suffix-length, last 500 bytes of the file.
//
struct RangeRequest {
  bool is_range_request = false;
  int64_t start = -1;  // Set to "-1" for suffix-length.
  int64_t end = -1;    // Set to "-1" for open-range.
};

// Responses to range requests use HTTP "206 Partial Content", a "Content-Range"
// header, and "Accept-Ranges" to indicate support. For example:
//
//   Accept-Ranges: bytes
//   Content-Range: bytes 1000-2000/3412
//   Content-Length: 1001
//
// The range is inclusive, the total length can be replaced by "*" if unknown.
// The server responds with "416 Range Not Satisfiable" if the requested range
// cannot be satisfied, and a "Content-Range" header with the total size.
//
//   HTTP/1.1 416 Range Not Satisfiable
//   Content-Range: bytes */3412
//
struct RangeResponse {
  int64_t start = -1;  // Set to "-1" if not satisfiable.
  int64_t end = -1;    // Set to "-1" if not satisfiable.
  int64_t total = -1;
};

// Reads the range header from the request and converts it to the struct.
// Returns a status error if the header syntax is malformed.
util::StatusOr<RangeRequest> ParseRangeRequest(const HttpRequest& request);
util::StatusOr<RangeRequest> ParseRangeRequest(std::string_view range_header);

// Generates a RangeResponse from the request and an input stream. Returns an
// error if `is_range_request` is false. First, the total size of the stream
// is determined. If the request is not satisfiable, only the total file size
// is initialized. Otherwise, `start`, `end` and `total` are set, and the input
// stream is positioned at the start of the range.
util::StatusOr<RangeResponse> GenerateRangeResponse(const RangeRequest& request,
                                                    std::istream& input);

// Returns the "Content-Range" header text from a range response.
// This includes cases where the range is not satisfiable.
std::string GetContentRangeHeader(const RangeResponse& response);

}  // namespace net

#endif  // SRC_SERVER_NET_HTTP_RANGE_REQUEST_H_
