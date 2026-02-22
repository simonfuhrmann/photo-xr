#include "src/server/net/http_range_request.h"

#include <cstdint>

#include "src/server/net/http_request.h"
#include "src/server/util/status_or.h"
#include "src/server/util/string_utils.h"

namespace net {

util::StatusOr<RangeRequest> ParseRangeRequest(const HttpRequest& request) {
  std::string_view header = request.GetRequestHeader("range");
  if (header.empty()) return RangeRequest();  // Not a range request.
  return ParseRangeRequest(header);
}

util::StatusOr<RangeRequest> ParseRangeRequest(std::string_view header) {
  // Validate and skip the unit prefix
  std::string_view prefix = "bytes=";
  if (header.substr(0, prefix.size()) != prefix) {
    return util::InvalidArgumentError("Invalid range unit");
  }
  std::string_view spec = header.substr(prefix.size());

  // Split the string at the hyphen. Extactly two results are expected.
  constexpr bool kKeepEmpty = true;
  const std::vector<std::string> parts = util::StrSplit(spec, '-', kKeepEmpty);
  if (parts.size() != 2) {
    return util::InvalidArgumentError("Invalid range format");
  }

  // Range cannot be empty (e.g., "bytes=-").
  if (parts[0].empty() && parts[1].empty()) {
    return util::InvalidArgumentError("Invalid range format");
  }

  // Parse parts into integers. Negative values are invalid.
  RangeRequest result;
  result.is_range_request = true;
  if (!parts[0].empty()) {
    if (!util::StrToInt(parts[0], &result.start) || result.start < 0) {
      return util::InvalidArgumentError("Invalid range start");
    }
  }
  if (!parts[1].empty()) {
    if (!util::StrToInt(parts[1], &result.end) || result.end < 0) {
      return util::InvalidArgumentError("Invalid range end");
    }
  }

  // The range must be positive.
  if (result.start >= 0 && result.end >= 0 && result.start > result.end) {
    return util::InvalidArgumentError("Invalid range values");
  }

  return result;
}

util::StatusOr<RangeResponse> GenerateRangeResponse(const RangeRequest& request,
                                                    std::istream& input) {
  // Check the input request.
  if (!request.is_range_request) {
    return util::InvalidArgumentError("Not a range request");
  }
  if (request.start < 0 && request.end < 0) {
    return util::InvalidArgumentError("Invalid range request");
  }
  // Check the input stream.
  if (input.bad()) {
    return util::UnknownError("Input stream is in bad state");
  }

  // Get the total size of the file.
  input.seekg(0, std::ios::end);
  const std::streampos total_size = input.tellg();
  if (total_size < 0) {
    return util::UnknownError("Failed to determine input stream size");
  }

  RangeResponse response;
  response.total = total_size;

  // An empty file cannot satisfy any range request.
  if (total_size == 0) {
    response.start = -1;
    response.end = -1;
    return response;
  }

  // If the start/end is out of bounds, the request is not satisfiable.
  if ((request.end >= 0 && request.end >= total_size) ||
      (request.start >= 0 && request.start >= total_size)) {
    response.start = -1;
    response.end = -1;
    input.seekg(0, std::ios::beg);  // Rewind the input stream.
    return response;
  }

  // If the start is negative, this is a suffix-length request.
  if (request.start < 0) {
    response.start = total_size - request.end;
    response.end = static_cast<int64_t>(total_size) - 1;
    input.seekg(response.start, std::ios::beg);
    return response;
  }

  // If the end is negative, this is an open-range request.
  if (request.end < 0) {
    response.start = request.start;
    response.end = static_cast<int64_t>(total_size) - 1;
    input.seekg(response.start, std::ios::beg);
    return response;
  }

  // Otherwise, this is a full-range request.
  response.start = request.start;
  response.end = request.end;
  input.seekg(response.start, std::ios::beg);
  return response;
}

std::string GetContentRangeHeader(const RangeResponse& response) {
  if (response.total < 0) {
    return util::StrCat("bytes */*");
  }
  if (response.start == -1 || response.end == -1) {
    return util::StrCat("bytes */", response.total);
  }
  return util::StrCat("bytes ", response.start, "-", response.end, "/",
                       response.total);
}

}  // namespace net
