#include "src/server/util/base64.h"

#include <array>

namespace util {
namespace {

// The encoding table with all Base64 characters (plus null terminator).
constexpr char kEncodeTable[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// The decoding table maps one Base64 character to its 6-bit value. Invalid
// characters are mapped to 0xFF. The table is generated at compile time.
constexpr std::array<uint8_t, 256> kDecodeTable = [] {
  std::array<uint8_t, 256> table{};
  for (size_t i = 0; i < 256; ++i) table[i] = 0xFF;
  for (size_t i = 0; i < 64; ++i) table[kEncodeTable[i]] = i;
  return table;
}();

}  // namespace

std::string Base64Encode(const uint8_t* data, size_t size) {
  // Return the empty string if the input is empty.
  if (data == nullptr || size == 0) return {};

  const size_t out_len = 4 * ((size + 2) / 3);
  std::string out(out_len, '\0');

  size_t i = 0;  // Input index.
  size_t j = 0;  // Output index.
  while (i + 3 <= size) {
    // Read 3 bytes (24 bits) of the input.
    const uint32_t v = static_cast<uint32_t>(data[i]) << 16 |
                       static_cast<uint32_t>(data[i + 1]) << 8 |
                       static_cast<uint32_t>(data[i + 2]) << 0;
    // Encode as 4 characters (6 bits each).
    out[j++] = kEncodeTable[(v >> 18) & 0x3F];
    out[j++] = kEncodeTable[(v >> 12) & 0x3F];
    out[j++] = kEncodeTable[(v >> 6) & 0x3F];
    out[j++] = kEncodeTable[v & 0x3F];
    i += 3;
  }

  // There may be 1 or 2 bytes left to encode. This requires padding.
  if (i < size) {
    // Read the remaining 1 or 2 bytes and pad with zeros.
    uint32_t v = static_cast<uint32_t>(data[i]) << 16;
    if (i + 1 < size) v |= static_cast<uint32_t>(data[i + 1]) << 8;
    // Encode as 4 characters (6 bits each), padding with 1 or 2 '='.
    out[j++] = kEncodeTable[(v >> 18) & 0x3F];
    out[j++] = kEncodeTable[(v >> 12) & 0x3F];
    out[j++] = (i + 1 < size) ? kEncodeTable[(v >> 6) & 0x3F] : '=';
    out[j++] = '=';
  }
  return out;
}

StatusOr<std::vector<uint8_t>> Base64Decode(const std::string& base64) {
  // Return an empty vector if the input is empty.
  if (base64.empty()) return std::vector<uint8_t>{};

  // The input must be a multiple of 4 characters.
  if (base64.size() % 4 != 0) {
    return InvalidArgumentError("Invalid base64 length");
  }

  // Check number of padding characters.
  size_t padding = 0;
  if (base64[base64.size() - 1] == '=') padding++;
  if (base64[base64.size() - 2] == '=') padding++;

  // Estimate the output size.
  const size_t out_len = base64.size() / 4 * 3 - padding;
  std::vector<uint8_t> out(out_len);

  size_t i = 0;  // Input index.
  size_t j = 0;  // Output index.
  while (i < base64.size()) {
    // Process 4 input chars at a time and pack 6 bits each into `v`.
    uint32_t v = 0;
    for (int k = 0; k < 4; ++k, ++i) {
      const uint8_t c = base64[i];
      // Padding is only allowed in the padding region.
      if (c == '=' && i >= base64.size() - padding) {
        v = v << 6;  // Pad with 6 bit zeros.
        continue;
      }
      // Decode the character to its 6-bit value, reject invalid characters.
      const uint8_t decoded = kDecodeTable[c];
      if (decoded == 0xFF) {
        return InvalidArgumentError("Invalid base64 character");
      }
      v = (v << 6) | decoded;
    }

    // Extract 3 bytes (24 bits) from `v` to output. Stop extracting output
    // characters if the desired length is reached, which ignores padding.
    if (j < out_len) out[j++] = (v >> 16) & 0xFF;
    if (j < out_len) out[j++] = (v >> 8) & 0xFF;
    if (j < out_len) out[j++] = v & 0xFF;
  }
  return out;
}

}  // namespace util
