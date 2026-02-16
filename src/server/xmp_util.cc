#include "src/server/xmp_util.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/util/status_or.h"

namespace server {
namespace {

// Explicit headers INCLUDING the null terminator
static constexpr char kXmpMainHeader[] = "http://ns.adobe.com/xap/1.0/\0";
static constexpr char kXmpExtHeader[] = "http://ns.adobe.com/xmp/extension/\0";
static constexpr size_t kXmpMainHeaderSize = sizeof(kXmpMainHeader) - 1;
static constexpr size_t kXmpExtHeaderSize = sizeof(kXmpExtHeader) - 1;

// Helper: read big-endian 32-bit
uint32_t ReadBE32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

}  // namespace

util::StatusOr<std::string> ExtractXmpFromJpeg(std::string_view jpeg) {
  struct ExtChunk {
    uint32_t offset;
    std::string data;
  };

  std::string main_xmp;

  std::string ext_guid;
  uint32_t ext_full_length = 0;
  std::vector<ExtChunk> ext_chunks;

  const uint8_t* data = reinterpret_cast<const uint8_t*>(jpeg.data());
  size_t size = jpeg.size();

  if (size < 4 || data[0] != 0xFF || data[1] != 0xD8) {
    return util::AbortedError("Not a JPEG file");
  }

  size_t pos = 2;  // Skip SOI

  while (pos + 4 <= size) {
    if (data[pos] != 0xFF) break;

    uint8_t marker = data[pos + 1];

    // Stop at SOS
    if (marker == 0xDA) break;

    uint16_t segment_length =
        (uint16_t(data[pos + 2]) << 8) | uint16_t(data[pos + 3]);

    if (segment_length < 2) return util::AbortedError("Invalid segment length");

    size_t payload_start = pos + 4;
    size_t payload_size = segment_length - 2;

    if (payload_start + payload_size > size)
      return util::AbortedError("Truncated JPEG");

    if (marker == 0xE1) {  // APP1
      const char* seg_data =
          reinterpret_cast<const char*>(data + payload_start);

      // ---- Standard XMP ----
      if (payload_size >= kXmpMainHeaderSize &&
          std::memcmp(seg_data, kXmpMainHeader, kXmpMainHeaderSize) == 0) {
        main_xmp.assign(seg_data + kXmpMainHeaderSize,
                        payload_size - kXmpMainHeaderSize);
      }

      // ---- Extended XMP ----
      else if (payload_size >= kXmpExtHeaderSize &&
               std::memcmp(seg_data, kXmpExtHeader, kXmpExtHeaderSize) == 0) {
        size_t p = kXmpExtHeaderSize;

        if (p + 32 + 4 + 4 > payload_size) continue;  // malformed

        std::string guid(seg_data + p, 32);
        p += 32;

        uint32_t full_length =
            ReadBE32(reinterpret_cast<const uint8_t*>(seg_data + p));
        p += 4;

        uint32_t offset =
            ReadBE32(reinterpret_cast<const uint8_t*>(seg_data + p));
        p += 4;

        if (p > payload_size) continue;

        std::string chunk(seg_data + p, payload_size - p);

        ext_guid = guid;
        ext_full_length = full_length;

        ext_chunks.push_back({offset, std::move(chunk)});
      }
    }

    pos += 2 + segment_length;
  }

  // ---- Reassemble Extended XMP properly ----
  if (!ext_chunks.empty()) {
    if (ext_full_length == 0)
      return util::AbortedError("Extended XMP has zero length");

    std::string extended(ext_full_length, '\0');

    for (const auto& chunk : ext_chunks) {
      if (chunk.offset + chunk.data.size() > extended.size()) {
        return util::AbortedError("Extended XMP chunk out of bounds");
      }

      std::copy(chunk.data.begin(), chunk.data.end(),
                extended.begin() + chunk.offset);
    }

    main_xmp += extended;
  }

  return main_xmp;
}

}  // namespace server
