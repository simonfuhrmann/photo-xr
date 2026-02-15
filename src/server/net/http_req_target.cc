#include "src/server/net/http_req_target.h"

#include <string_view>

#include "src/server/util/string_utils.h"

namespace net {

HttpReqTarget::HttpReqTarget(std::string_view request_target)
    : target_(request_target) {
  // Find start of query string.
  const size_t qpos = target_.find('?');
  if (qpos == std::string::npos) {
    path_ = target_;
    return;
  }

  // Path is everything before '?'.
  path_ = std::string_view(target_).substr(0, qpos);
  query_ = std::string_view(target_).substr(qpos + 1);

  // Parse query string, e.g., "key=value&key2=value2".
  size_t pos = qpos + 1;
  while (pos < target_.size()) {
    size_t amp = target_.find('&', pos);
    if (amp == std::string::npos) amp = target_.size();

    const size_t eq = target_.find('=', pos);
    if (eq == std::string::npos || eq > amp) {
      std::string_view key(target_.data() + pos, amp - pos);
      query_params_.emplace(key, std::string_view{});
    } else {
      std::string_view key(target_.data() + pos, eq - pos);
      std::string_view val(target_.data() + eq + 1, amp - eq - 1);
      query_params_.emplace(key, val);
    }
    pos = amp + 1;
  }
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
