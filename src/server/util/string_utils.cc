#include "src/server/util/string_utils.h"

#include <sstream>
#include <string>

namespace util {
namespace {
bool IsNewline(char c) { return c == '\n' || c == '\r'; }
bool IsWhitespace(char c) { return c == ' ' || c == '\t'; }
bool IsWhitespaceOrNewline(char c) { return IsNewline(c) || IsWhitespace(c); }
}  // namespace

std::vector<std::string> StrSplit(std::string_view input, char delim,
                                  bool keep_empty) {
  std::vector<std::string> output;
  std::string current;
  current.reserve(128);
  for (const char& c : input) {
    if (c == delim) {
      if (!keep_empty && current.empty()) continue;
      output.emplace_back() = std::move(current);
      continue;
    }
    current.push_back(c);
  }
  if (!current.empty() || keep_empty) {
    output.emplace_back() = std::move(current);
  }
  return output;
}

bool StrToInt(std::string_view str, int* value) {
  std::stringstream ss;
  ss << str;
  ss >> *value;
  return ss.eof() && !ss.fail();
}

void ToLowercase(std::string& string) {
  for (char& c : string) {
    c = std::tolower(c);
  }
}

std::string TrimNewlines(std::string_view input) {
  size_t end = input.size();
  while (end > 0 && IsNewline(input[end - 1])) end--;
  return std::string(input.substr(0, end));
}

std::string TrimWhitespaces(std::string_view input) {
  size_t start = 0;
  size_t end = input.size();
  while (start < end && IsWhitespace(input[start])) start++;
  while (end > start && IsWhitespace(input[end - 1])) end--;
  return std::string(input.substr(start, end - start));
}

std::string TrimString(std::string_view input) {
  size_t start = 0;
  size_t end = input.size();
  while (start < end && IsWhitespaceOrNewline(input[start])) start++;
  while (end > start && IsWhitespaceOrNewline(input[end - 1])) end--;
  return std::string(input.substr(start, end - start));
}

bool HasPrefix(std::string_view input, std::string_view prefix) {
  return input.substr(0, prefix.size()) == prefix;
}

}  // namespace util
