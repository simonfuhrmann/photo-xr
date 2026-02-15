#include "src/server/api_handler.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/net/http_req_target.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/json.h"

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
  }

  return util::AbortedError("Invalid API request");
}

util::Status ApiHandler::HandleAlbumRequest(
    net::HttpRequest& request, const net::HttpReqTarget& target) const {
  const std::string_view album_path = target.GetParam("path");
  const std::string full_path =
      util::StrCat(options_.photos_root, "/", std::string(album_path));

  // Normalize the path and ensure it is under the photos root directory.
  const std::string normalized_path = util::GetNormalizedPath(full_path);
  if (!util::HasPrefix(normalized_path, options_.photos_root)) {
    return util::AbortedError("Invalid album path");
  }

  // Check if the album path exists and is a directory.
  if (!fs::exists(normalized_path) || !fs::is_directory(normalized_path)) {
    return util::AbortedError("Album not found");
  }

  // Read the directory contents.
  std::vector<std::string> albums;
  std::vector<std::string> photos;
  for (const auto& entry : fs::directory_iterator(normalized_path)) {
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
  for (const auto& [name, value]: options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyBody(json.Serialize(), /*copy_data=*/true);
  return request.Reply();
}

}  // namespace server
