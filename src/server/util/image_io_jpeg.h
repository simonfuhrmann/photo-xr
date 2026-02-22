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

util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::string_view filename);
util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   std::istream& input);
util::StatusOr<ImageData> JpegRead(const JpegReadOptions& options,
                                   const char* data, size_t size);

}  // namespace util

#endif  // SRC_SERVER_UTIL_IMAGE_IO_JPEG_H_
