#include "src/server/album_cache.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "src/server/file_utils.h"
#include "src/server/media_type.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/image_io_jpeg.h"
#include "src/server/util/status_or.h"

namespace server {
namespace {

namespace fs = std::filesystem;
using FsEntry = fs::directory_entry;
using FsTime = std::filesystem::file_time_type;

std::string GetCacheFile(std::string_view album_dir) {
  return std::string(album_dir) + "/.photoxr.cache";
}

int64_t ChronoTimeToMillis(const FsTime& time) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             time.time_since_epoch())
      .count();
}

FsTime ChronoTimeFromMillis(int64_t millis) {
  return FsTime(std::chrono::milliseconds(millis));
}

FsTime GetLastModifiedTime(const FsEntry& entry) {
  return entry.last_write_time();
}

bool IsGphotoJpeg(const FsEntry& entry) {
  util::JpegReadOptions options;
  options.include_image_data = false;
  options.include_xmp_data = true;
  std::string_view path = entry.path().c_str();
  const util::StatusOr<util::ImageData> image = JpegRead(options, path);
  if (!image.ok()) return false;

  const std::string& xmp_meta = image->xmp_metadata;
  return xmp_meta.find("xmlns:GImage") != std::string::npos &&
         xmp_meta.find("HasExtendedXMP") != std::string::npos;
}

MediaType GetMediaType(const FsEntry& entry) {
  if (IsVideoFile(entry)) return MediaType::VIDEO_SBS;
  if (IsSplatFile(entry)) return MediaType::GEOMETRY_SPLAT;
  if (IsImageFile(entry)) {
    if (IsGphotoJpeg(entry)) return MediaType::IMAGE_GPHOTO;
    return MediaType::IMAGE_SBS;
  }
  return MediaType::UNKNOWN;
}

}  // namespace

AlbumCache::AlbumCache(std::string_view album_dir) : album_dir_(album_dir) {
  ReadCacheFromFile();
}

AlbumCache::~AlbumCache() { WriteCacheToFile(); }

AlbumCache::CacheData AlbumCache::Get(std::string_view filename) {
  // TODO: Check if the file has changed.
  const auto it = cache_.find(std::string(filename));
  if (it != cache_.end()) {
    return it->second;
  }

  const FsEntry entry(fs::path(album_dir_) / filename);

  CacheData cache_data;
  cache_data.media_type = GetMediaType(entry);
  cache_data.last_modified = GetLastModifiedTime(entry);
  cache_[std::string(filename)] = cache_data;
  return cache_data;
}

void AlbumCache::WriteCacheToFile() {
  if (cache_.empty()) return;

  const std::string cache_file = GetCacheFile(album_dir_);
  std::ofstream out(cache_file);
  if (!out) {
    std::cerr << "Failed to open cache file for writing: " << cache_file
              << std::endl;
    return;
  }
  for (const auto& [filename, data] : cache_) {
    const int64_t ts = ChronoTimeToMillis(data.last_modified);
    out << filename << " " << ts << " " << MediaTypeToString(data.media_type)
        << "\n";
  }
  out.close();
}

void AlbumCache::ReadCacheFromFile() {
  // Cache file may not exist on the first run.
  const std::string cache_file = GetCacheFile(album_dir_);
  std::ifstream in(cache_file);
  if (!in) return;

  std::string filename;
  int64_t last_modified;
  std::string media_type;
  while (in >> filename >> last_modified >> media_type) {
    cache_[filename] = {
        .last_modified = ChronoTimeFromMillis(last_modified),
        .media_type = MediaTypeFromString(media_type),
    };
  }
  in.close();
}

}  // namespace server
