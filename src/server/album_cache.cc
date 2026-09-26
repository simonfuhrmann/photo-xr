#include "src/server/album_cache.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
using FsTime = fs::file_time_type;

constexpr std::string_view kCacheFile = ".photoxr.cache";
constexpr int kCacheVersion = 1;

int64_t ChronoTimeToMillis(const FsTime& time) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             time.time_since_epoch())
      .count();
}

FsTime ChronoTimeFromMillis(int64_t millis) {
  return FsTime(std::chrono::milliseconds(millis));
}

bool ReadCacheFileEntry(std::ifstream& in, fs::path* filename,
                        AlbumCache::CacheData* data) {
  if (in.eof()) return false;

  std::string filename_str;
  int64_t last_modified;
  std::string media_type;
  std::getline(in, filename_str, '\t');
  in >> last_modified >> media_type;
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  if (in.eof()) return false;

  *filename = filename_str;
  data->last_modified = ChronoTimeFromMillis(last_modified);
  data->media_type = MediaTypeFromString(media_type);
  return true;
}

void WriteCacheFileEntry(std::ostream& out, const fs::path& filename,
                         const AlbumCache::CacheData& data) {
  out << filename.c_str() << "\t" << ChronoTimeToMillis(data.last_modified)
      << "\t" << MediaTypeToString(data.media_type) << "\n";
}

fs::path GetCacheFile(const fs::path& media_root) {
  return media_root / kCacheFile;
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

AlbumCache::AlbumCache(std::string_view media_root)
    : media_root_(media_root), cache_file_(GetCacheFile(media_root)) {
  ReadCacheFromFile();
}

AlbumCache::~AlbumCache() { WriteCacheToFile(); }

const std::filesystem::path AlbumCache::GetMediaRoot() const {
  return media_root_;
}

util::StatusOr<AlbumCache::CacheData> AlbumCache::Get(
    const fs::path& filename) const {
  // TODO: Check if the file has changed.
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    const auto it = cache_.find(filename);
    if (it != cache_.end()) {
      return it->second;
    }
  }

  const FsEntry entry(media_root_ / filename);
  CacheData cache_data;
  cache_data.media_type = GetMediaType(entry);
  cache_data.last_modified = GetLastModifiedTime(entry);

  std::scoped_lock<std::mutex> lock(mutex_);
  cache_[filename] = cache_data;
  return cache_data;
}

void AlbumCache::WriteCacheToFile() {
  std::scoped_lock<std::mutex> lock(mutex_);
  if (cache_.empty()) return;

  std::cout << "Writing cache to " << cache_file_ << " ...\n";
  std::ofstream out(cache_file_);
  if (!out) {
    std::cerr << "Failed to open cache for writing: " << cache_file_ << "\n";
    return;
  }

  // Write the version and entries to the cache.
  out << "version " << kCacheVersion << "\n";
  for (const auto& [filename, data] : cache_) {
    WriteCacheFileEntry(out, filename, data);
  }
  out.close();
}

void AlbumCache::ReadCacheFromFile() {
  // Cache file may not exist on the first run.
  std::cout << "Reading cache from " << cache_file_ << " ...\n";
  std::ifstream in(cache_file_);
  if (!in) return;

  // Read the version from the cache file. If version is too low, discard cache.
  std::string version_str;
  int version_int;
  in >> version_str >> version_int;
  in.ignore(1, '\n');
  std::cout << "Version: " << version_str << " " << version_int << "\n";
  if (version_str != "version") {
    std::cout << "Cache is not versioned, discarding\n";
    return;
  }

  // Discard cache if the cache version does not match.
  if (version_int < kCacheVersion) {
    std::cout << "Cache version is outdated, discarding\n";
    return;
  } else if (version_int > kCacheVersion) {
    std::cout << "Cache version is too recent, discarding\n";
    return;
  }

  {
    std::scoped_lock<std::mutex> lock(mutex_);
    fs::path filename;
    CacheData cache_data;
    while (ReadCacheFileEntry(in, &filename, &cache_data)) {
      cache_[filename] = cache_data;
    }
  }
  in.close();
}

}  // namespace server
