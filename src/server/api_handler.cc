#include "src/server/api_handler.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/album_cache.h"
#include "src/server/file_utils.h"
#include "src/server/net/http_req_target.h"
#include "src/server/util/file_utils.h"
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
  }

  return util::AbortedError("Invalid API request");
}

util::StatusOr<std::string> ApiHandler::GetLocalPath(
    std::string_view req_path) const {
  // Normalize the path and ensure it is under the photos root directory.
  std::string local_path =
      util::GetNormalizedPath(util::StrCat(options_.media_root, "/", req_path));
  if (!util::HasPrefix(local_path, options_.media_root)) {
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

}  // namespace server
