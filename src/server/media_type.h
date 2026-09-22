#ifndef SRC_SERVER_MEDIA_TYPE_H_
#define SRC_SERVER_MEDIA_TYPE_H_

#include <string>
#include <string_view>

namespace server {

enum class MediaType {
  UNKNOWN,
  IMAGE_SBS,
  IMAGE_GPHOTO,
  VIDEO_SBS,
  GEOMETRY_SPLAT,
};

std::string MediaTypeToString(MediaType type);
MediaType MediaTypeFromString(std::string_view string);

}  // namespace server

#endif  // SRC_SERVER_MEDIA_TYPE_H_
