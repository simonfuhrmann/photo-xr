#include "src/server/util/json.h"

#include <charconv>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "src/server/util/string_utils.h"

namespace util {
namespace {

bool IsWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool IsNumberChar(char c) {
  return std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' ||
         c == '-';
}

}  // namespace

//
// Json::Stream implementation.
//

// Input stream wrapper that keeps track of row and column.
class Json::Stream {
 public:
  Stream(std::istream& in);
  char Get();
  char Peek();
  bool Good();
  bool Eof();
  void SkipWhitespaces();

 public:
  std::istream& in_;
  int row_ = 0;
  int col_ = 0;
};

Json::Stream::Stream(std::istream& in) : in_(in) {}

char Json::Stream::Get() {
  if (!Good()) return in_.peek();
  const char c = in_.get();
  if (c == '\n') {
    row_ += 1;
    col_ = -1;
  } else {
    col_ += 1;
  }
  return c;
}

char Json::Stream::Peek() { return in_.peek(); }

bool Json::Stream::Good() { return !!in_; }

bool Json::Stream::Eof() { return in_.eof(); }

void Json::Stream::SkipWhitespaces() {
  while (Good() && IsWhitespace(Peek())) Get();
}

//
// Json::Value implementation.
//

const Json::Value* Json::Value::GetObjectValue(std::string_view key) const {
  const Object* obj = GetObject();
  if (obj == nullptr) return nullptr;
  const auto iter = obj->find(key);
  if (iter == obj->end()) return nullptr;
  return &iter->second;
}

const Json::Null* Json::Value::GetObjectNull(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetNull();
}

const Json::Bool* Json::Value::GetObjectBool(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetBool();
}

const Json::Number* Json::Value::GetObjectNumber(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetNumber();
}

const Json::String* Json::Value::GetObjectString(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetString();
}

const Json::Array* Json::Value::GetObjectArray(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetArray();
}

const Json::Object* Json::Value::GetObjectObject(std::string_view key) const {
  const Value* value = GetObjectValue(key);
  return value == nullptr ? nullptr : value->GetObject();
}

//
// JsonSerializer implementation.
//

class JsonSerializer {
 public:
  JsonSerializer(int indent = 0);

  std::string Serialize(const Json::Value& value);
  std::string operator()(const Json::Null& value);
  std::string operator()(const Json::Bool& value);
  std::string operator()(const Json::Number& value);
  std::string operator()(const Json::String& value);
  std::string operator()(const Json::Array& value);
  std::string operator()(const Json::Object& value);

 private:
  void IndentMore();
  void IndentLess();
  std::string Indent() const;
  std::string EscapeString(std::string_view str) const;
  std::string FormatNumber(double number) const;

  int indent_ = 0;
};

JsonSerializer::JsonSerializer(int indent) : indent_(indent) {}

std::string JsonSerializer::Serialize(const Json::Value& value) {
  return std::visit(*this, value);
}

std::string JsonSerializer::operator()(const Json::Null& value) {
  return "null";
}

std::string JsonSerializer::operator()(const Json::Bool& value) {
  return value ? "true" : "false";
}

std::string JsonSerializer::operator()(const Json::Number& value) {
  return FormatNumber(value);
}

std::string JsonSerializer::operator()(const Json::String& value) {
  return EscapeString(value);
}

std::string JsonSerializer::operator()(const Json::Array& value) {
  if (value.empty()) return "[]";

  std::string out = "[\n";
  IndentMore();

  for (size_t i = 0; i < value.size(); ++i) {
    out += Indent() + Serialize(value[i]);
    if (i + 1 < value.size()) out += ",";
    out += "\n";
  }

  IndentLess();
  out += Indent() + "]";
  return out;
}

std::string JsonSerializer::operator()(const Json::Object& value) {
  if (value.empty()) return "{}";

  std::string out = "{\n";
  IndentMore();

  for (auto iter = value.begin(); iter != value.end(); iter++) {
    const std::string& key = iter->first;
    const Json::Value& val = iter->second;
    out += Indent() + EscapeString(key) + ": " + Serialize(val);
    if (auto next = iter; ++next != value.end()) out += ",";
    out += "\n";
  }

  IndentLess();
  out += Indent() + "}";
  return out;
}

void JsonSerializer::IndentMore() { indent_ += 2; }

void JsonSerializer::IndentLess() { indent_ -= 2; }

std::string JsonSerializer::Indent() const { return std::string(indent_, ' '); }

std::string JsonSerializer::EscapeString(std::string_view str) const {
  std::string escaped = "\"";
  for (char c : str) {
    // clang-format off
    switch (c) {
      case '\"': escaped += "\\\""; break;
      case '\\': escaped += "\\\\"; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default: escaped += c;
    }
    // clang-format on
  }
  escaped += "\"";
  return escaped;
}

std::string JsonSerializer::FormatNumber(double number) const {
  std::ostringstream oss;
  oss << std::setprecision(15) << std::noshowpoint << number;
  return oss.str();
}

//
// Json implementation.
//

Json::Status Json::Parse(std::string_view text) {
  std::istringstream in{std::string(text)};  // Copy :(
  return Parse(in);
}

Json::Status Json::Parse(std::istream& stream) {
  value_ = nullptr;
  Stream in(stream);
  return ParseJson(in, &value_);
}

Json::Status Json::ParseFile(std::string_view filename) {
  std::ifstream in(filename.data());
  if (!in.good()) return Status{false, "Error opening file", -1, -1};
  return Parse(in);
}

std::string Json::Serialize() const { return Serialize(value_); }

std::string Json::Serialize(const Value& value) {
  JsonSerializer serializer;
  return serializer.Serialize(value);
}

// Parses a JSON document. This is identical to ParseValue() except that no
// trailing characters are permitted.
Json::Status Json::ParseJson(Stream& in, Value* value) {
  const Status status = ParseValue(in, value);
  if (!status.success) return status;
  in.SkipWhitespaces();
  if (!in.Eof()) return {false, "Trailing characters", in.row_, in.col_};
  return {};
}

// Parses a JSON value. A value can be `null`, a boolean (`true` or `false`),
// a number, a quoted string, an array (`[...]`) or an object (`{...}`).
Json::Status Json::ParseValue(Stream& in, Value* value) {
  in.SkipWhitespaces();
  const char c = in.Peek();
  if (c == '"') {
    return ParseString(in, value);
  } else if (c == '{') {
    return ParseObject(in, value);
  } else if (c == '[') {
    return ParseArray(in, value);
  } else if (c == 't' || c == 'f') {
    return ParseBool(in, value);
  } else if (c == 'n') {
    return ParseNull(in, value);
  } else if (c == '-' || std::isdigit(c)) {
    return ParseNumber(in, value);
  }

  return Status{
      .success = false,
      .error_msg = util::StrCat("Unexpected character: ", c),
      .error_row = in.row_,
      .error_col = in.col_,
  };
}

Json::Status Json::ParseNull(Stream& in, Value* value) {
  const int row = in.row_;
  const int col = in.col_;
  for (const char c : std::string_view("null")) {
    if (c != in.Get()) return {false, "Unexpected token", row, col};
  }
  *value = Null();
  return {};
}

Json::Status Json::ParseBool(Stream& in, Value* value) {
  const int row = in.row_;
  const int col = in.col_;
  const bool bool_value = in.Peek() == 't';
  for (const char c : std::string_view(bool_value ? "true" : "false")) {
    if (c != in.Get()) return {false, "Unexpected token", row, col};
  }
  *value = Bool(bool_value);
  return {};
}

Json::Status Json::ParseNumber(Stream& in, Value* value) {
  const int row = in.row_;
  const int col = in.col_;

  // Add number-looking characters to `buffer`.
  std::string buffer;
  buffer.reserve(16);
  while (!in.Eof()) {
    if (!IsNumberChar(in.Peek())) break;
    buffer += in.Get();
    continue;
  }

  // Convert to number.
  double number;
  const char* ptr_begin = buffer.c_str();
  const char* ptr_end = ptr_begin + buffer.size();
  std::from_chars_result result = std::from_chars(ptr_begin, ptr_end, number);
  if (result.ptr != ptr_end) {
    return {false, "Number parse error", row, col};
  }

  *value = Number(number);
  return {};
}

Json::Status Json::ParseString(Stream& in, Value* value) {
  if (in.Get() != '"') {
    return {false, "String is not quoted", in.row_, in.col_ - 1};
  }

  std::string result;
  while (!in.Eof()) {
    const char c = in.Get();
    if (in.Eof()) break;

    // End of string.
    if (c == '"') {
      *value = String(std::move(result));
      return {};
    }

    // Escaped character.
    if (c == '\\') {
      const char esc = in.Get();
      if (in.Eof()) break;
      // clang-format off
      switch (esc) {
        case 'b': result += '\b'; break;
        case 'f': result += '\f'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        default: result += esc;
      }
      // clang-format on
      continue;
    }

    if (c < 0x20) {
      return {false, "Unsupported control character", in.row_, in.col_ - 1};
    }

    result += c;
  }

  // The loop above is broken on unexpected EOF.
  return {false, "Unexpected EOF", in.row_, in.col_ - 1};
}

Json::Status Json::ParseArray(Stream& in, Value* value) {
  // Consume array start.
  if (in.Get() != '[') {
    return {false, "Missing array start", in.row_, in.col_ - 1};
  }

  // Check for empty array.
  in.SkipWhitespaces();
  if (in.Peek() == ']') {
    in.Get();
    *value = Array();
    return {};
  }

  Array array;
  while (true) {
    // Parse array value.
    in.SkipWhitespaces();
    if (in.Eof()) return {false, "Unexpected EOF", in.row_, in.col_ - 1};
    const Status status = ParseValue(in, &array.emplace_back());
    if (!status.success) return status;

    // Parse the separator.
    in.SkipWhitespaces();
    const char sep = in.Get();
    if (sep == ']') break;
    if (sep == ',') continue;

    return {false, "Invalid array separator", in.row_, in.col_ - 1};
  }

  *value = Array(std::move(array));
  return {};
}

Json::Status Json::ParseObject(Stream& in, Value* value) {
  // Consume object start.
  if (in.Get() != '{') {
    return {false, "Missing object start", in.row_, in.col_ - 1};
  }

  // Check for empty object.
  in.SkipWhitespaces();
  if (in.Peek() == '}') {
    in.Get();
    *value = Object();
    return {};
  }

  Object object;
  while (true) {
    in.SkipWhitespaces();
    if (in.Eof()) return {false, "Unexpected EOF", in.row_, in.col_ - 1};

    // Parse the object key.
    int row = in.row_;
    int col = in.col_;
    Value key;
    const Status key_status = ParseString(in, &key);
    if (!key_status.success) return {false, "Invalid object key", row, col};

    // Parse the colon.
    in.SkipWhitespaces();
    if (in.Eof()) return {false, "Unexpected EOF", in.row_, in.col_ - 1};
    if (in.Get() != ':') {
      return {false, "Unexpected key/value separator", in.row_, in.col_ - 1};
    }

    // Parse the object value.
    in.SkipWhitespaces();
    if (in.Eof()) return {false, "Unexpected EOF", in.row_, in.col_ - 1};
    Value value;
    const Status value_status = ParseValue(in, &value);
    if (!value_status.success) return value_status;

    // Add key/value to object.
    object[*key.GetString()] = std::move(value);

    // Check key/value separator.
    in.SkipWhitespaces();
    if (in.Eof()) return {false, "Unexpected EOF", in.row_, in.col_ - 1};
    const char sep = in.Get();
    if (sep == '}') break;
    if (sep == ',') continue;
    return {false, "Invalid key/value separator", in.row_, in.col_ - 1};
  }

  *value = std::move(object);
  return {};
}

}  // namespace util
