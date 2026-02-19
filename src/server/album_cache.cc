#include "src/server/album_cache.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

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

util::StatusOr<std::string> GetStereoType(const FsEntry& entry) {
  util::LoadJpegOptions options;
  options.include_image_data = false;
  options.include_xmp_data = true;
  std::string_view path = entry.path().c_str();
  ASSIGN_OR_RETURN(const util::ImageData image, JpegRead(options, path));
  if (image.xmp_metadata.find("xmlns:GImage") != std::string::npos &&
      image.xmp_metadata.find("HasExtendedXMP") != std::string::npos) {
    return std::string("gphoto");
  }
  // Default to side-by-side if no XMP metadata found.
  return std::string("sbs");
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
  util::StatusOr<std::string> stereo_type = GetStereoType(entry);

  CacheData cache_data;
  cache_data.stereo_type = stereo_type.ok() ? *stereo_type : "sbs";
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
    out << filename << " " << ts << " " << data.stereo_type << "\n";
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
  std::string stereo_type;
  while (in >> filename >> last_modified >> stereo_type) {
    cache_[filename] = {ChronoTimeFromMillis(last_modified), stereo_type};
  }
  in.close();
}

}  // namespace server
