#include "src/server/file_utils.h"

#include <algorithm>
#include <string>

namespace server {
namespace {

namespace fs = std::filesystem;

std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}
}  // namespace

bool IsImageFile(const fs::directory_entry& entry) {
  const fs::path& p = entry.path();
  const std::string ext = ToLower(p.extension().string());
  return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp";
}

bool IsVideoFile(const fs::directory_entry& entry) {
  const fs::path& p = entry.path();
  const std::string ext = ToLower(p.extension().string());
  return ext == ".mp4" || ext == ".webm";
}

bool IsMediaFile(const std::filesystem::directory_entry& entry) {
  return IsImageFile(entry) || IsVideoFile(entry) || IsSplatFile(entry);
}

bool IsSplatFile(const std::filesystem::directory_entry& entry) {
  const fs::path& p = entry.path();
  const std::string ext = ToLower(p.extension().string());
  return ext == ".ply";
}

}  // namespace server
