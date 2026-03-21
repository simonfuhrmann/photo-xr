#ifndef SRC_SERVER_NET_URL_ENCODING_H_
#define SRC_SERVER_NET_URL_ENCODING_H_

#include <string>
#include <string_view>

#include "src/server/util/status_or.h"

namespace net {

struct UrlDecodeOptions {
  bool allow_encoded_slash = false;  // Allow encoded '/' (%2F) characters.
  bool allow_encoded_null = false;   // Allow encoded NUL (%00) characters.
};

// Decodes a URL-encoded string. Returns an error if the input contains invalid
// percent-encoding or if it contains disallowed characters based on the
// provided options. The recommendation for URL decoding is to never allow
// encoded null characters, and to not allow encoded slash characters in paths.
util::StatusOr<std::string> UrlDecode(const UrlDecodeOptions& options,
                                      std::string_view input);

}  // namespace net

#endif  // SRC_SERVER_NET_URL_ENCODING_H_
