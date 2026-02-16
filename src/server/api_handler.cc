#include "src/server/api_handler.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/net/http_req_target.h"
#include "src/server/util/base64.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/json.h"
#include "src/server/util/status_or.h"
#include "src/server/xmp_util.h"

namespace server {
namespace {
namespace fs = std::filesystem;

std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

bool IsImageFile(const fs::directory_entry& entry) {
  const fs::path& p = entry.path();
  const std::string ext = ToLower(p.extension().string());
  return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp";
}

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

  // Read the directory contents.
  std::vector<std::string> albums;
  std::vector<std::string> photos;
  for (const auto& entry : fs::directory_iterator(local_path)) {
    if (entry.is_directory()) {
      albums.push_back(entry.path().filename().string());
    } else if (IsImageFile(entry)) {
      photos.push_back(entry.path().filename().string());
    }
  }

  // Sort the sub-albums and photos alphabetically.
  std::sort(albums.begin(), albums.end());
  std::sort(photos.begin(), photos.end());

  // Convert to JSON: directories to albums and files to photos.
  util::Json::Array entries;
  for (const std::string& album : albums) {
    util::Json::Object album_entry;
    album_entry["type"] = "album";
    album_entry["name"] = album;
    entries.push_back(album_entry);
  }
  for (const std::string& photo : photos) {
    util::Json::Object photo_entry;
    photo_entry["type"] = "photo";
    photo_entry["name"] = photo;
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
  const std::string_view eye = target.GetParam("eye");
  if (eye != "left" && eye != "right") {
    return util::AbortedError("Invalid eye parameter");
  }
  // Get the local path of the photo.
  const std::string_view req_path = target.GetParam("path");
  ASSIGN_OR_RETURN(const std::string local_path, GetLocalPath(req_path));

  const bool is_left_eye = (eye == "left");
  return is_left_eye ? HandlePhotoLeftEyeRequest(request, local_path)
                     : HandlePhotoRightEyeRequest(request, local_path);
}

util::Status ApiHandler::HandlePhotoLeftEyeRequest(
    net::HttpRequest& request, std::string_view local_path) const {
  // For the left eye, just return the original JPEG data. It would be nice
  // to strip the extra XMP metadata for the right eye to save bandwidth.
  ASSIGN_OR_RETURN(const std::string photo_data, util::ReadFile(local_path));
  request.SetReplyStatus(net::HttpStatus::CODE_200_OK);
  request.SetReplyContentType("image/jpeg");
  for (const auto& [name, value] : options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyBody(photo_data, /*copy_data=*/false);
  return request.Reply();
}

util::Status ApiHandler::HandlePhotoRightEyeRequest(
    net::HttpRequest& request, std::string_view local_path) const {
  // For the right eye, read the full JPEG, find the base64 encoded JPEG in the
  // XMP metadata, decode it and return the data.
  ASSIGN_OR_RETURN(const std::string photo_data, util::ReadFile(local_path));
  ASSIGN_OR_RETURN(std::string xmp_data, ExtractXmpFromJpeg(photo_data));

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
  ASSIGN_OR_RETURN(const std::string decoded_data,
                   util::Base64Decode(base64_data));

  // Send the binary JPEG data as the response.
  request.SetReplyStatus(net::HttpStatus::CODE_200_OK);
  request.SetReplyContentType("image/jpeg");
  for (const auto& [name, value] : options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyBody(decoded_data, /*copy_data=*/false);
  return request.Reply();
}

}  // namespace server
