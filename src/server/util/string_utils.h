#ifndef SRC_UTIL_STRING_UTILS_H_
#define SRC_UTIL_STRING_UTILS_H_

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace util {

// Splits the `input` string at the given `delim` delimiter. If `keep_empty` is
// true, then empty strings between two subsequent delimiters is kept.
std::vector<std::string> StrSplit(std::string_view input, char delim = ' ',
                                  bool keep_empty = false);

// Concatenates the given arguments into a std::string.
template <typename... Args>
std::string StrCat(Args&&... args);

// Converts the given string to an integer. Returns false on error.
bool StrToInt(std::string_view str, int* value);
bool StrToInt(std::string_view str, int64_t* value);

// Converts each character to lower-case.
void ToLowercase(std::string& string);

// Removes '\r' and '\n' line breaking characters from the end of `input`.
std::string TrimNewlines(std::string_view input);

// Removes whitespace characters (' ', '\t') from the start and end of `input`.
std::string TrimWhitespaces(std::string_view input);

// Removes whitespace and newline characters from the start and end of `input`.
std::string TrimString(std::string_view input);

// Returns true if `input` starts with `prefix`.
bool HasPrefix(std::string_view input, std::string_view prefix);

// Inline implementation.

namespace detail {

// Base case for recursion that does nothing.
inline void AppendToStream(std::ostringstream&) {}

// Recursion that appends a single argument.
template <typename T, typename... Rest>
void AppendToStream(std::ostringstream& oss, T&& value, Rest&&... rest) {
  oss << std::forward<T>(value);
  AppendToStream(oss, std::forward<Rest>(rest)...);
}

}  // namespace detail

template <typename... Args>
std::string StrCat(Args&&... args) {
  std::ostringstream oss;
  util::detail::AppendToStream(oss, std::forward<Args>(args)...);
  return oss.str();
}

}  // namespace util

#endif  // SRC_UTIL_STRING_UTILS_H_
