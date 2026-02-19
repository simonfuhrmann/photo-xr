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

struct LoadJpegOptions {
  bool include_image_data = true;
  bool include_xmp_data = false;
};

util::StatusOr<ImageData> JpegRead(const LoadJpegOptions& options,
                                   std::string_view filename);
util::StatusOr<ImageData> JpegRead(const LoadJpegOptions& options,
                                   std::istream& input);

}  // namespace util
