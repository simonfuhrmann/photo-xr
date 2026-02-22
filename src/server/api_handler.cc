#include "src/server/api_handler.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/album_cache.h"
#include "src/server/file_utils.h"
#include "src/server/net/http_req_target.h"
#include "src/server/stitch_vr180.h"
#include "src/server/util/base64.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/image_io_jpeg.h"
#include "src/server/util/json.h"
#include "src/server/util/status_or.h"

namespace server {
namespace {

namespace fs = std::filesystem;
using FsEntry = fs::directory_entry;

}  // namespace

ApiHandler::ApiHandler(const Options& options) : options_(options) {}

util::Status ApiHandler::Handle(net::HttpRequest& request) const {
  // Only handle requests that start with "/api/".
  const net::HttpReqTarget target(request.GetRequestTarget());
  if (!util::HasPrefix(target.GetPath(), "/api/")) {
    return util::AbortedError("Not an API request");
  }

  // Only allow GET requests for now.
  if (request.GetRequestMethod() != net::HttpMethod::GET) {
    request.SetReplyStatus(net::HttpStatus::CODE_405_METHOD_NOT_ALLOWED);
    request.SetReplyContentType("text/plain");
    request.SetReplyBody("Only GET method is allowed", /*copy_data=*/false);
    return request.Reply();
  }

  if (target.GetPath() == "/api/album") {
    return HandleAlbumRequest(request, target);
  } else if (target.GetPath() == "/api/photo") {
    return HandlePhotoRequest(request, target);
  }

  return util::AbortedError("Invalid API request");
}

util::StatusOr<std::string> ApiHandler::GetLocalPath(
    std::string_view req_path) const {
  // Normalize the path and ensure it is under the photos root directory.
  std::string local_path = util::GetNormalizedPath(
      util::StrCat(options_.photos_root, "/", req_path));
  if (!util::HasPrefix(local_path, options_.photos_root)) {
    return util::AbortedError("Invalid path");
  }
  // Make sure the path exists in the file system.
  if (!fs::exists(local_path)) {
    return util::AbortedError("Path not found");
  }
  return local_path;
}

util::Status ApiHandler::HandleAlbumRequest(
    net::HttpRequest& request, const net::HttpReqTarget& target) const {
  const std::string_view album_path = target.GetParam("path");
  ASSIGN_OR_RETURN(const std::string local_path, GetLocalPath(album_path));

  // Check if the album path is a directory.
  if (!fs::is_directory(local_path)) {
    return util::AbortedError("Album not found");
  }

  AlbumCache album_cache(local_path);

  // Read the directory contents.
  std::vector<FsEntry> albums;
  std::vector<FsEntry> files;
  for (const FsEntry& entry : fs::directory_iterator(local_path)) {
    if (entry.is_directory()) {
      albums.push_back(entry);
    } else if (IsMediaFile(entry)) {
      files.push_back(entry);
    }
  }

  // Sort the sub-albums and media alphabetically.
  std::sort(albums.begin(), albums.end());
  std::sort(files.begin(), files.end());

  // Convert to JSON: directories to albums and files to photos.
  util::Json::Array entries;
  for (const FsEntry& album : albums) {
    util::Json::Object album_entry;
    album_entry["type"] = "album";
    album_entry["name"] = album.path().filename().string();
    entries.push_back(album_entry);
  }
  for (const FsEntry& file : files) {
    const std::string file_path = file.path().filename().string();
    AlbumCache::CacheData cache_data = album_cache.Get(file_path);
    util::Json::Object photo_entry;
    photo_entry["type"] = IsVideoFile(file) ? "video" : "photo";
    photo_entry["name"] = file_path;
    photo_entry["stereo"] = cache_data.stereo_type;
    entries.push_back(photo_entry);
  }
  util::Json::Object json_response;
  json_response["path"] = std::string(album_path);
  json_response["entries"] = entries;
  util::Json json(json_response);

  // Send the response as a JSON object.
  request.SetReplyStatus(net::HttpStatus::CODE_200_OK);
  request.SetReplyContentType("application/json");
  for (const auto& [name, value] : options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyBody(json.Serialize(), /*copy_data=*/true);
  return request.Reply();
}

util::Status ApiHandler::HandlePhotoRequest(
    net::HttpRequest& request, const net::HttpReqTarget& target) const {
  // Get the local path of the photo, and read into memory.
  const std::string_view req_path = target.GetParam("path");
  ASSIGN_OR_RETURN(const std::string local_path, GetLocalPath(req_path));

  // Read the JPEG including XMP metadata.
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

util::StatusOr<std::string> ApiHandler::GetEyeDataGphoto(
    const std::string& local_path, bool is_left_eye) const {
  // For the left eye, just return the full JPEG data.
  // TODO: Strip metadata, specifally the second eye in the XMP.
  if (is_left_eye) {
    return util::ReadFile(local_path);
  }

  // For the right eye, read the XMP from the JPEG, and decode the base64
  // encoded right-eye JPEG data in the XMP.
  util::JpegReadOptions options;
  options.include_image_data = false;
  options.include_xmp_data = true;
  ASSIGN_OR_RETURN(util::ImageData image, JpegRead(options, local_path));
  std::string& xmp_data = image.xmp_ext_metadata;

  // Extract the base64-encoded JPEG in the GImage:Data field.
  constexpr std::string_view kXmpDataPrefix = "GImage:Data=\"";
  const size_t xmp_pos = xmp_data.find(kXmpDataPrefix);
  if (xmp_pos == std::string_view::npos) {
    return util::AbortedError("No embedded data found in the photo");
  }

  // Compute the base64 start and end position in the XMP data.
  const size_t base64_start = xmp_pos + kXmpDataPrefix.size();
  size_t base64_end = xmp_data.find_first_of('"', base64_start);
  if (base64_end == std::string_view::npos) {
    return util::AbortedError("Invalid XMP metadata format");
  }

  // Some JPEG/XMP encoders do not properly pad the base64 data to multiples
  // of 4 bytes. Sneak in the padding, overwriting original data in place (the
  // data is not used otherwise).
  while ((base64_end - base64_start) % 4 != 0) {
    if (base64_end + 1 >= xmp_data.size()) break;
    xmp_data[base64_end] = '=';
    base64_end += 1;
  }

  // Decode the base64 data.
  const std::string_view xmp_data_view(xmp_data);
  const std::string_view base64_data =
      xmp_data_view.substr(base64_start, base64_end - base64_start);
  return util::Base64Decode(base64_data);
}

util::StatusOr<std::string> ApiHandler::GetEyeDataSbs(
    const std::string& local_path, bool is_left_eye) const {
  // Decode the JPEG data and split the left and right eye.
  // TODO: implement this. Requires JPEG decoding, splitting, re-encoding.
  return util::AbortedError("SBS format not supported yet");
}

}  // namespace server
