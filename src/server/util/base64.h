#ifndef SRC_SERVER_UTIL_BASE64_H_
#define SRC_SERVER_UTIL_BASE64_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "src/server/util/status_or.h"

namespace util {

// Encodes the input data in base64 and returns the result as a string.
// Enconding complies with RFC 4648 with the alphabet [A-Za-z0-9+/] and the
// padding character '='. Each 3 bytes of the input are encoded as 4 base64
// characters in the output. If the input is one or two less character than a
// multiple of 3, the output is padded with one or two '=' characters,
// respectively. If `size` is zero, an empty string is returned.
std::string Base64Encode(std::string_view data);
std::string Base64Encode(const uint8_t* data, size_t size);

// Decodes the input base64 string (inverse of `Base64Encode`) and returns the
// result as a string (which can contain binary data and null characters). If
// the base64 input string contains invalid characters, or is not properly
// padded to a multiple of 4 characters, an error is returned.
StatusOr<std::string> Base64Decode(std::string_view base64);

}  // namespace util

#endif  // SRC_SERVER_UTIL_BASE64_H_
