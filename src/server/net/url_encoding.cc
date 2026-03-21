#include "src/server/net/url_encoding.h"

namespace net {
namespace {
int HexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
}  // namespace

util::StatusOr<std::string> UrlDecode(const UrlDecodeOptions& options,
                                      std::string_view input) {
  // The output is guaranteed to be at most the size of the input.
  std::string output;
  output.reserve(input.size());

  for (size_t i = 0; i < input.size(); ++i) {
    // Append non-encoded characters directly.
    char c = input[i];
    if (c != '%') {
      output.push_back(c);
      continue;
    }

    // Require two hex digits.
    if (i + 2 >= input.size()) {
      return util::InvalidArgumentError("Incomplete percent encoding");
    }
    const int hi = HexValue(input[i + 1]);
    const int lo = HexValue(input[i + 2]);
    if (hi < 0 || lo < 0) {
      return util::InvalidArgumentError("Invalid percent encoding");
    }
    const char decoded = static_cast<char>((hi << 4) | lo);

    // Check if the character is allowed according to options.
    if (!options.allow_encoded_slash && decoded == '/') {
      return util::InvalidArgumentError("Encoded '/' not allowed");
    }
    if (!options.allow_encoded_null && decoded == '\0') {
      return util::InvalidArgumentError("Encoded NUL not allowed");
    }

    // Append the decoded character.
    output.push_back(decoded);
    i += 2;
  }

  return output;
}

}  // namespace net
