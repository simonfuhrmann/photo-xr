#include "src/server/api_handler.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/album_cache.h"
#include "src/server/file_utils.h"
#include "src/server/media_type.h"
#include "src/server/net/http_req_target.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/json.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace server {
namespace {

namespace fs = std::filesystem;
using FsEntry = fs::directory_entry;

}  // namespace

ApiHandler::ApiHandler(const Options& options) : options_(options) {}

util::Status ApiHandler::Handle(net::HttpRequest& request) const {
  net::HttpReqTarget target;
  RETURN_IF_ERROR(target.Parse(request.GetRequestTarget()));
  const auto& path_components = target.GetPathComponents();

  // Only handle requests that start with "/api/".
  if (path_components.empty() || path_components[0] != "api") {
    return util::AbortedError("Not an API request");
  }

  // Only allow GET requests for now.
  if (request.GetRequestMethod() != net::HttpMethod::GET) {
    request.SetReplyStatus(net::HttpStatus::CODE_405_METHOD_NOT_ALLOWED);
    request.SetReplyContentType("text/plain");
    request.SetReplyBody("Only GET method is allowed", /*copy_data=*/false);
    return request.Reply();
  }

  if (path_components.size() == 2 && path_components[1] == "album") {
    return HandleAlbumRequest(request, target);
  }

  return util::AbortedError("Invalid API request");
}

util::StatusOr<fs::path> GetReqAlbumPath(const net::HttpReqTarget& target) {
  const std::string_view path = target.GetParam("path");
  if (path.empty() || path[0] != '/') {
    return util::InvalidArgumentError("Album path must be absolute");
  }
  return fs::path(path).lexically_normal().relative_path();
}

util::Status ApiHandler::HandleAlbumRequest(
    net::HttpRequest& request, const net::HttpReqTarget& target) const {
  const AlbumCache& album_cache = *options_.album_cache;
  const fs::path& media_root = album_cache.GetMediaRoot();
  ASSIGN_OR_RETURN(const fs::path rel_album_path, GetReqAlbumPath(target));
  const fs::path abs_album_path = media_root / rel_album_path;

  // Ensure the request path is under the media root.
  const std::string norm_path = util::GetNormalizedPath(abs_album_path.c_str());
  if (!util::HasPrefix(norm_path, media_root.c_str())) {
    return util::AbortedError("Invalid path");
  }
  // Ensure the path exists in the file system.
  if (!fs::exists(abs_album_path)) {
    return util::AbortedError("Path not found");
  }
  // Check if the album path is a directory.
  if (!fs::is_directory(abs_album_path)) {
    return util::AbortedError("Album not found");
  }

  // Read the directory contents.
  std::vector<FsEntry> albums;
  std::vector<FsEntry> files;
  for (const FsEntry& entry : fs::directory_iterator(abs_album_path)) {
    if (entry.is_directory()) {
      albums.push_back(entry);
    } else if (IsMediaFile(entry)) {
      files.push_back(entry);
    }
  }

  // Sort the sub-albums and media alphabetically.
  std::sort(albums.begin(), albums.end());
  std::sort(files.begin(), files.end());

  // Convert to JSON: directories to albums and files to media.
  util::Json::Array entries;
  for (const FsEntry& album : albums) {
    util::Json::Object album_entry;
    album_entry["name"] = album.path().filename();
    album_entry["media"] = "ALBUM";
    entries.push_back(album_entry);
  }
  for (const FsEntry& file : files) {
    const fs::path filename = file.path().filename();
    util::StatusOr<AlbumCache::CacheData> cache_data =
        album_cache.Get(rel_album_path / filename);
    if (!cache_data.ok()) continue;
    if (cache_data->media_type == MediaType::UNKNOWN) continue;

    util::Json::Object photo_entry;
    photo_entry["name"] = filename;
    photo_entry["media"] = MediaTypeToString(cache_data->media_type);
    entries.push_back(photo_entry);
  }
  util::Json::Object json_response;
  json_response["path"] = "/" + rel_album_path.generic_string();
  json_response["entries"] = std::move(entries);
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
