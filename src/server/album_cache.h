#ifndef SRC_SERVER_ALBUM_CACHE_H_
#define SRC_SERVER_ALBUM_CACHE_H_

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace server {

class AlbumCache {
 public:
  using FsTime = std::filesystem::file_time_type;

  struct CacheData {
    FsTime last_modified;
    std::string stereo_type;
  };

  explicit AlbumCache(std::string_view album_dir);
  ~AlbumCache();

  CacheData Get(std::string_view filename);

 private:
  void WriteCacheToFile();
  void ReadCacheFromFile();

  std::string album_dir_;
  std::map<std::string, CacheData> cache_;
};

}  // namespace server

#endif  // SRC_SERVER_ALBUM_CACHE_H_
