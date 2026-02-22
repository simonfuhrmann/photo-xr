#ifndef SRC_SERVER_UTIL_IMAGE_IO_JPEG_H_
#define SRC_SERVER_UTIL_IMAGE_IO_JPEG_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/util/status_or.h"

namespace util {

struct ImageData {
  int width;
  int height;
  int channels;
  std::vector<uint8_t> data;
  std::string xmp_metadata;
  std::string xmp_ext_metadata;
};

struct JpegReadOptions {
  bool include_image_data = true;
  bool include_xmp_data = false;
};

struct JpegWriteOptions {
  int quality = 90;
};

// Reading JPEG from file, stream, or memory.
util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::string_view filename);
util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::istream& input);
util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   const char* data, size_t size);

// Write JPEG to file, stream, or memory. When using streams, and if JPEG
// compression fails, bytes may have already been written to the output stream.
util::Status JpegWrite(const JpegWriteOptions& options, const ImageData& image,
                       std::string_view filename);
util::Status JpegWrite(const JpegWriteOptions& options, const ImageData& image,
                       std::ostream& output);
util::Status JpegWrite(const JpegWriteOptions& options, const ImageData& image,
                       std::vector<uint8_t>& output);

}  // namespace util

#endif  // SRC_SERVER_UTIL_IMAGE_IO_JPEG_H_
