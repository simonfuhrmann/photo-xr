#include "src/server/util/file_utils.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifndef _WIN32
#include <climits>
#include <cstdlib>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif  // _WIN32

#include "src/server/util/string_utils.h"

namespace util {
namespace {

util::StatusOr<std::string> ReadFileInternal(std::string_view filename,
                                             bool align) {
  // std::ifstream can fail in unexpected ways, esp. on non-regular files.
  if (!std::filesystem::is_regular_file(filename)) {
    return util::NotFoundError(StrCat("Not a regular file: ", filename));
  }
  // Seek to the end of the file and use tellg() to get the file size.
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    return util::NotFoundError(StrCat("Failed to open file: ", filename));
  }
  const std::istream::pos_type file_size = file.tellg();
  if (file.fail() || file_size == std::istream::pos_type(-1)) {
    return util::NotFoundError(StrCat("Failed to get file size: ", filename));
  }

  // Allocate a buffer that is 32-bit aligned.
  std::string buffer;
  if (align) {
    buffer.resize(GetAlignedSize(file_size), '\0');
  } else {
    buffer.resize(file_size);
  }

  // Seek to the beginning and read the file.
  file.seekg(0);
  if (file.fail()) {
    return util::CancelledError(StrCat("Failed to seek file: ", filename));
  }
  file.read(buffer.data(), file_size);
  if (file.fail()) {
    return util::CancelledError(StrCat("Failed to read file: ", filename));
  }
  return buffer;
}

}  // namespace

size_t GetAlignedSize(size_t file_size) {
  if (file_size % 4 == 0) return file_size;
  return file_size / 4 * 4 + 4;
}

util::StatusOr<std::string> ReadFile(std::string_view filename) {
  return ReadFileInternal(filename, /*align=*/false);
}

util::StatusOr<std::string> ReadFileAligned(std::string_view filename) {
  return ReadFileInternal(filename, /*align=*/true);
}

util::Status WriteFile(std::string_view filename, std::string_view data) {
  if (std::filesystem::exists(filename)) {
    return util::CancelledError(StrCat("File already exists: ", filename));
  }
  std::ofstream out(filename.data(), std::ios::binary | std::ios::out);
  if (!out.is_open()) {
    return util::CancelledError(StrCat("Error opening file: ", filename));
  }
  out.write(data.data(), data.size());
  if (out.fail()) {
    return util::CancelledError(StrCat("Writing failed: ", filename));
  }
  out.close();
  return util::OkStatus();
}

util::StatusOr<std::string> GetCanonicalPath(std::string_view path) {
  namespace fs = std::filesystem;
  std::error_code error_code;
  const fs::path canonical = fs::canonical(path, error_code);
  if (error_code) return util::FailedPreconditionError(error_code.message());
  return canonical.string();
}

std::string GetNormalizedPath(std::string_view path) {
  return std::filesystem::path(path).lexically_normal();
}

std::string_view GetFileExtension(std::string_view filename) {
  // Check the position of the last dot, return empty string if not found.
  const size_t dot_pos = filename.find_last_of('.');
  if (dot_pos == std::string_view::npos) {
    return filename.substr(filename.size());
  }

  // Check for the position of the last slash. If a slash exists and the last
  // dot is before the slash, return the empty string.
  const size_t slash_pos = filename.find_last_of('/');
  if (slash_pos != std::string_view::npos && dot_pos < slash_pos) {
    return filename.substr(filename.size());
  }

  // Return the component after the last dot.
  return filename.substr(dot_pos + 1);
}

util::StatusOr<std::string> GetCurrentPath() {
  std::error_code ec;
  const std::filesystem::path path = std::filesystem::current_path(ec);
  if (ec) return util::InternalError(ec.message());
  return path.string();
}

}  // namespace util
