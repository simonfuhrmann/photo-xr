#include <string>
#include <string_view>

#include "src/server/util/status_or.h"

namespace server {

// Extracts the XMP metadata from a JPEG file. The XMP may be split over
// multiple APP1 segments, and must be properly reassembled.
// Disclaimer: This is a fair bit vibe-coded with ChatGPT.
util::StatusOr<std::string> ExtractXmpFromJpeg(std::string_view jpeg);

}  // namespace server
