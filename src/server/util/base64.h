#ifndef SRC_SERVER_UTIL_BASE64_H_
#define SRC_SERVER_UTIL_BASE64_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "src/server/util/status_or.h"

namespace util {

// Encodes the input data in base64 and returns the result as a string.
// Enconding complies with RFC 4648 with the alphabet [A-Za-z0-9+/] and the
// padding character '='. Each 3 bytes of the input are encoded as 4 base64
// characters in the output. If the input is one or two less character than a
// multiple of 3, the output is padded with one or two '=' character,
// respectively. If `size` is zero, an empty string is returned.
std::string Base64Encode(const uint8_t* data, size_t size);

// Decodes the input base64 string as the inverse of `Base64Encode` and returns
// the result as a byte vector. If the base64-decoded string contains invalid
// characters, and error is returned.
StatusOr<std::vector<uint8_t>> Base64Decode(const std::string& base64);

}  // namespace util

#endif  // SRC_SERVER_UTIL_BASE64_H_
