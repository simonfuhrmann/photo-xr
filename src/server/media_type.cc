#include "src/server/media_type.h"

namespace server {

std::string MediaTypeToString(MediaType type) {
  if (type == MediaType::IMAGE_SBS) return "IMAGE_SBS";
  if (type == MediaType::IMAGE_GPHOTO) return "IMAGE_GPHOTO";
  if (type == MediaType::VIDEO_SBS) return "VIDEO_SBS";
  if (type == MediaType::GEOMETRY_SPLAT) return "GEOMETRY_SPLAT";
  return "UNKNOWN";
}

MediaType MediaTypeFromString(std::string_view string) {
  if (string == "IMAGE_SBS") return MediaType::IMAGE_SBS;
  if (string == "IMAGE_GPHOTO") return MediaType::IMAGE_GPHOTO;
  if (string == "VIDEO_SBS") return MediaType::VIDEO_SBS;
  if (string == "GEOMETRY_SPLAT") return MediaType::GEOMETRY_SPLAT;
  return MediaType::UNKNOWN;
}

}  // namespace server
