#include "src/server/media_handler.h"

#include <filesystem>
#include <iostream>
#include <string>

#include "src/server/album_cache.h"
#include "src/server/stitch_vr180.h"
#include "src/server/util/image_io_jpeg.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace server {
namespace {

net::HttpStaticFileHandler::Options GetFileHandlerOptions(
    const MediaHandler::Options& options) {
  net::HttpStaticFileHandler::Options file_options;
  file_options.root_dir = options.media_root;
  file_options.path_prefix = "/media";
  file_options.reply_headers = options.reply_headers;
  return file_options;
}

}  // namespace

MediaHandler::MediaHandler(const Options& options)
    : net::HttpStaticFileHandler(GetFileHandlerOptions(options)),
      options_(options) {}

util::Status MediaHandler::Handle(net::HttpRequest& request) const {
  // Only GET requests are supported.
  if (request.GetRequestMethod() != net::HttpMethod::GET) {
    return util::InvalidArgumentError("Only GET requests are supported");
  }

  ASSIGN_OR_RETURN(const std::string& local_path,
                   GetLocalFilePath(request.GetRequestTarget()));

  // Video files and SBS photo files can be served as-is via the file handler.
  // VR180 files (where the second eye is in the XMP metadata) must be stitched
  // to a SBS JPEG.
  const std::filesystem::path file_path(local_path);
  const std::filesystem::path parent_path = file_path.parent_path();
  const std::filesystem::path filename = file_path.filename();
  AlbumCache cache(parent_path.string());
  AlbumCache::CacheData cache_data = cache.Get(filename.string());
  if (cache_data.stereo_type == "sbs") {
    return net::HttpStaticFileHandler::Handle(request);
  }

  // This is a VR180 file. Read the JPEG including XMP metadata.
  util::JpegReadOptions options;
  options.include_image_data = true;
  options.include_xmp_data = true;
  ASSIGN_OR_RETURN(util::ImageData image, util::JpegRead(options, local_path));

  // Stitch the image to a side-by-size equirect.
  ASSIGN_OR_RETURN(util::ImageData stitched_image, StitchVr180(image));

  // Encode the stitched image back to JPEG format.
  util::JpegWriteOptions write_options;
  write_options.quality = 90;
  std::string jpeg_data;
  RETURN_IF_ERROR(util::JpegWrite(write_options, stitched_image, jpeg_data));

  // Send the response to the client.
  request.SetReplyStatus(net::HttpStatus::CODE_200_OK);
  request.SetReplyContentType("image/jpeg");
  for (const auto& [name, value] : options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyBody(jpeg_data, /*copy_data=*/false);
  return request.Reply();
}

}  // namespace server
