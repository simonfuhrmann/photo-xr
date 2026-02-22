#include "src/server/util/image_io_jpeg.h"
#include "src/server/util/status_or.h"

namespace server {

// Relevant metadata extracted from the XMP of the JPEG.
struct PhotoSphereMeta {
  int crop_width = 0;
  int crop_height = 0;
  int full_width = 0;
  int full_height = 0;
};

// Extracts PhotoSphere metadata from the XMP metadata of a JPEG image.
// This is exposed for testing.
util::StatusOr<PhotoSphereMeta> ExtractPhotoSphereMeta(std::string_view xml);

// Stitches a side-by-side equirect image from a VR180 JPEG file. The left eye
// is assumed in the main image data, the right is in the extended XMP metadata
// as a base64 encoded JPEG.
util::StatusOr<util::ImageData> StitchVr180(const util::ImageData& image_data);

}  // namespace server
