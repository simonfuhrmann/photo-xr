#ifndef SRC_SERVER_ALBUM_CACHE_H_
#define SRC_SERVER_ALBUM_CACHE_H_

#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <string_view>

#include "src/server/media_type.h"
#include "src/server/util/status_or.h"

namespace server {

class AlbumCache {
 public:
  using FsTime = std::filesystem::file_time_type;

  struct CacheData {
    FsTime last_modified;
    MediaType media_type;
  };

  // A single cache file is created in the media root, called `.photoxr.cache`.
  explicit AlbumCache(std::string_view media_root);
  ~AlbumCache();

  const std::filesystem::path GetMediaRoot() const;

  // Returns the cache data for `filename` which must be relative to the media
  // root. If the media type is already cached, it is returned from the cache.
  // Otherwise, the media type is determined and cached, then returned. If the
  // file exists but the media type is unknown, `MediaType::UNKNOWN` is
  // returned. Otherwise, an error status is returned.
  util::StatusOr<CacheData> Get(const std::filesystem::path& filename) const;

 private:
  void WriteCacheToFile();
  void ReadCacheFromFile();

  std::filesystem::path media_root_;
  std::filesystem::path cache_file_;

  mutable std::mutex mutex_;
  mutable std::map<std::filesystem::path, CacheData> cache_;
};

}  // namespace server

#endif  // SRC_SERVER_ALBUM_CACHE_H_
