#ifndef SRC_NET_HTTP_REQ_TARGET_H_
#define SRC_NET_HTTP_REQ_TARGET_H_

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/util/status.h"

namespace net {

// A class to parse HTTP's request target and provide access to the components.
// HTTP's request target is the text between the HTTP method and the protocol.
//
// For example: "GET /index.html?user=alice&debug=1 HTTP/1.1"
//
//   - Request target: "/index.html?user=alice&debug=1"
//   - Request path: "/index.html"
//   - Query parameters: "user=alice&debug=1"
//
// Beacuse the client can encode any character using percent-encoding, it is
// important WHEN to decode '?', '&', '=', and '/' characters, to prevent
// ambiguity, which can lead to security vulnerabilities. Specifically, all of
// reserved delimiter characters must appear literally (unencoded) in the
// request target, and decoding happens between the segments.
class HttpReqTarget {
 public:
  using QueryParamsMap = std::map<std::string, std::string, std::less<>>;
  using PathComponents = std::vector<std::string>;

  HttpReqTarget() = default;

  // The algorithm to parse the request target is as follows:
  //
  //   1. Split path and query at the first '?' character
  //   2. Split the path at '/' characters, then URL-decode each path segment
  //   3. Normalize the path components, remove "." and process ".." segments
  //   4. Split the query at '&' characters into query parameters
  //   5. Split query parameters at the first '=' character into key/value pairs
  //   6. URL-decode each key and value in the query parameters
  //
  // Encoded '/' characters (%2F) in path components are rejected as errors.
  // Encoded '\0' characters (%00) anywhere are treated as errors.
  util::Status Parse(std::string_view request_target);

  // Full HTTP request target, including path and query parameters, potentially
  // included percent-decoded characters. This should rarely be used.
  std::string_view GetTarget() const;

  // The full path component of the request target, potentially including
  // percent-encoded characters, and unnormalized. This should rarely be used.
  std::string_view GetPath() const;

  // The full query component of the request target, potentially including
  // percent-encoded characters. This should rarely be used.
  std::string_view GetQuery() const;

  // Returns the decoded and normalized path segments.
  const PathComponents& GetPathComponents() const;

  // Returns the re-assembled, normalized, decoded request path.
  std::string GetNormalizedPath() const;

  // Returns the decoded query parameters.
  const QueryParamsMap& GetQueryParams() const;
  std::string_view GetParam(std::string_view key) const;
  int GetParamInt(std::string_view key, int default_value) const;

 private:
  std::string target_;      // Full copy of the request target.
  std::string_view path_;   // View into the path component.
  std::string_view query_;  // View into the query parameters.

  // List of decoded and normalized path components.
  PathComponents path_components_;

  // Map of decoded query parameters, e.g., "user" -> "alice".
  QueryParamsMap query_params_;
};

// Inline implementation.

inline std::string_view HttpReqTarget::GetTarget() const { return target_; }

inline std::string_view HttpReqTarget::GetPath() const { return path_; }

inline std::string_view HttpReqTarget::GetQuery() const { return query_; }

inline const HttpReqTarget::PathComponents& HttpReqTarget::GetPathComponents()
    const {
  return path_components_;
}

inline const HttpReqTarget::QueryParamsMap& HttpReqTarget::GetQueryParams()
    const {
  return query_params_;
}

}  // namespace net

#endif  // SRC_NET_HTTP_REQ_TARGET_H_
