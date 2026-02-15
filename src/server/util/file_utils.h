#ifndef SRC_UTIL_FILE_UTILS_H_
#define SRC_UTIL_FILE_UTILS_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "src/server/util/status_or.h"

namespace util {

// Returns the smallest number not smaller than size but divisible by 4.
std::size_t GetAlignedSize(std::size_t size);

// Returns the contents of the given file.
util::StatusOr<std::string> ReadFile(std::string_view filename);

// Same is `ReadFile()`, but aligns the start address of the data to a 4-byte
// boundary, and ensures the size of the string is divisible by 4 (i.e., the
// size of the string may be up to 3 bytes larger than the file size, and
// padded with null characters, '\0').
util::StatusOr<std::string> ReadFileAligned(std::string_view filename);

// Write the in-memory data contents of the given file name.
util::Status WriteFile(std::string_view filename, std::string_view data);

// Expands all symbolic links and resolves references to "." and "..", and
// extra directory separators to produce a canonical, absolute path. The path
// must exist, otherwise an error is returned.
util::StatusOr<std::string> GetCanonicalPath(std::string_view path);

// Resolves all references to "." and "..", and collapse extra directory
// separators to produce a normalized path. Uses pure lexical (string-based)
// normalization, without accessing the actual filesystem.
std::string GetNormalizedPath(std::string_view path);

// Returns the file extension for `filename`, as a string view into the
// argument. The extension is the suffix of `filename` after the last dot
// character '.', after the last directory separator '/'.
std::string_view GetFileExtension(std::string_view filename);

// Returns the current working directory.
util::StatusOr<std::string> GetCurrentPath();

}  // namespace util

#endif  // SRC_UTIL_FILE_UTILS_H_
