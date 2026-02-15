#ifndef SRC_NET_HTTP_REQ_TARGET_H_
#define SRC_NET_HTTP_REQ_TARGET_H_

#include <map>
#include <string>
#include <string_view>

namespace net {

// A class to parse HTTP's request target and provide access to the components.
// HTTP's request target is the text between the HTTP method and the protocol.
//
// For example: "GET /index.html?user=alice&debug=1 HTTP/1.1"
// - Request target: "/index.html?user=alice&debug=1"
// - Request path: "/index.html"
// - Query parameters: "user=alice&debug=1"
//
// The query parameters are represented as a key/value map.
class HttpReqTarget {
 public:
  HttpReqTarget(std::string_view request_target);

  std::string_view GetTarget() const;  // Full HTTP request target.
  std::string_view GetPath() const;    // Path component of the request target.
  std::string_view GetQuery() const;   // Query component of the request target.
  std::string_view GetParam(std::string_view key) const;
  int GetParamInt(std::string_view key, int default_value) const;

 private:
  std::string target_;      // Full copy of the request target.
  std::string_view path_;   // View into the path component.
  std::string_view query_;  // View into the query parameters.
  std::map<std::string_view, std::string_view> query_params_;
};

// Inline implementation.

inline std::string_view HttpReqTarget::GetTarget() const { return target_; }

inline std::string_view HttpReqTarget::GetPath() const { return path_; }

inline std::string_view HttpReqTarget::GetQuery() const { return query_; }

}  // namespace net

#endif  // SRC_NET_HTTP_REQ_TARGET_H_
