#include "src/server/net/http_req_target.h"

#include <list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "src/server/net/url_encoding.h"
#include "src/server/util/string_utils.h"

namespace net {

util::Status HttpReqTarget::Parse(std::string_view request_target) {
  // Reset state.
  target_.clear();
  path_ = std::string_view();
  query_ = std::string_view();
  path_components_.clear();
  query_params_.clear();

  if (request_target.empty()) {
    return util::InvalidArgumentError("Request target cannot be empty");
  }

  // Find start of query string. Path is everything before '?'.
  target_ = request_target;
  const size_t qpos = target_.find_first_of('?');
  if (qpos == std::string::npos) {
    path_ = target_;
  } else {
    path_ = std::string_view(target_).substr(0, qpos);
    query_ = std::string_view(target_).substr(qpos + 1);
  }

  // Split the path at '/' characters. The first component must be empty.
  std::vector<std::string> path_segments = util::StrSplit(path_, '/', true);
  if (path_segments.empty() || !path_segments[0].empty()) {
    return util::InvalidArgumentError("Path must start with '/'");
  }

  // URL-decode each path segment. No encoded '/' or '\0' chars allowed.
  UrlDecodeOptions decode_path_options;
  decode_path_options.allow_encoded_slash = false;
  decode_path_options.allow_encoded_null = false;
  for (std::string& segment : path_segments) {
    ASSIGN_OR_RETURN(segment, UrlDecode(decode_path_options, segment));
  }

  // Normalize the path segments (handle "." and "..").
  path_components_.reserve(path_segments.size());
  for (std::string& segment : path_segments) {
    // Ignore empty and "." segments.
    if (segment.empty() || segment == ".") continue;
    // Remove previous segment for ".." segments.
    if (segment == "..") {
      if (path_components_.empty()) {
        return util::InvalidArgumentError("Invalid path outside of root");
      }
      path_components_.pop_back();
      continue;
    }
    path_components_.push_back(std::move(segment));
  }

  // Parse query string, e.g., "key=value&key2=value2".
  std::list<std::pair<std::string_view, std::string_view>> query_params;
  size_t pos = 0;
  while (pos < query_.size()) {
    size_t amp = query_.find_first_of('&', pos);
    if (amp == std::string::npos) amp = query_.size();

    const size_t eq = query_.find_first_of('=', pos);
    if (eq == std::string::npos || eq > amp) {
      std::string_view key(query_.data() + pos, amp - pos);
      query_params.emplace_back(key, std::string_view{});
    } else {
      std::string_view key(query_.data() + pos, eq - pos);
      std::string_view val(query_.data() + eq + 1, amp - eq - 1);
      query_params.emplace_back(key, val);
    }
    pos = amp + 1;
  }

  // URL-decode each query key and value. No encoded '\0' chars allowed.
  UrlDecodeOptions decode_query_options;
  decode_query_options.allow_encoded_slash = true;
  decode_query_options.allow_encoded_null = false;
  for (const auto& [key, value] : query_params) {
    ASSIGN_OR_RETURN(std::string decoded_key,
                     UrlDecode(decode_query_options, key));
    ASSIGN_OR_RETURN(std::string decoded_value,
                     UrlDecode(decode_query_options, value));
    query_params_.emplace(std::move(decoded_key), std::move(decoded_value));
  }

  return util::OkStatus();
}

std::string HttpReqTarget::GetNormalizedPath() const {
  if (path_components_.empty()) return "/";

  int num_chars = 0;
  for (const std::string& component : path_components_) {
    num_chars += component.size() + 1;  // +1 for the '/' separator.
  }
  std::string normalized_path;
  normalized_path.reserve(num_chars);
  for (const std::string& component : path_components_) {
    normalized_path.append("/");
    normalized_path.append(component);
  }
  return normalized_path;
}

std::string_view HttpReqTarget::GetParam(std::string_view key) const {
  const auto iter = query_params_.find(key);
  if (iter == query_params_.end()) return std::string_view();
  return iter->second;
}

int HttpReqTarget::GetParamInt(std::string_view key, int default_value) const {
  std::string_view param = GetParam(key);
  if (param.empty()) return default_value;
  int value = 0;
  return util::StrToInt(param, &value) ? value : default_value;
}

}  // namespace net
