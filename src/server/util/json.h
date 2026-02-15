#ifndef SRC_UTIL_JSON_H_
#define SRC_UTIL_JSON_H_

#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace util {

// A simple JSON parser that favors simplicity over full RFC 8259 compliance
// and performance. In particular, number parsing is more lenient, and string
// parsing does not support unicode escapes and some control characters.
class Json {
 public:
  // Parsing result. If `success` is false, error, row and col are provided.
  struct [[nodiscard]] Status {
    bool success = true;
    std::string error_msg;
    int error_row = 0;
    int error_col = 0;
  };

  // All supported JSON types.
  struct Value;
  using Null = std::nullptr_t;
  using Bool = bool;
  using Number = double;
  using String = std::string;
  using Array = std::vector<Value>;
  using Object = std::map<std::string, Value, std::less<>>;

  // A JSON value holds exactly one of the possible JSON types, and is
  // represented using a std::variant. To access a specific type, use
  // Value::Get<T>(), which either returns the value of T or nullptr.
  struct Value : std::variant<Null, Bool, Number, String, Array, Object> {
    using variant::variant;  // Inherit constructors.

    template <typename T>
    const T* Get() const;

    const Null* GetNull() const;
    const Bool* GetBool() const;
    const Number* GetNumber() const;
    const String* GetString() const;
    const Array* GetArray() const;
    const Object* GetObject() const;

    const Value* GetObjectValue(std::string_view key) const;
    const Null* GetObjectNull(std::string_view key) const;
    const Bool* GetObjectBool(std::string_view key) const;
    const Number* GetObjectNumber(std::string_view key) const;
    const String* GetObjectString(std::string_view key) const;
    const Array* GetObjectArray(std::string_view key) const;
    const Object* GetObjectObject(std::string_view key) const;
  };

  Json() = default;
  explicit Json(const Value& value);

  // Parses a serialized JSON value from in-memory text, from a stream, or
  // from file. A JSON value is one of: An object, array, number, string, true,
  // false, or null.
  Status Parse(std::string_view text);
  Status Parse(std::istream& stream);
  Status ParseFile(std::string_view filename);

  // Returns the root of the JSON tree.
  const Value& GetRoot() const;

  // Serialize the JSON tree.
  std::string Serialize() const;
  static std::string Serialize(const Value& value);

 private:
  class Stream;

  Status ParseJson(Stream& in, Value* value);
  Status ParseValue(Stream& in, Value* value);
  Status ParseNull(Stream& in, Value* value);
  Status ParseBool(Stream& in, Value* value);
  Status ParseNumber(Stream& in, Value* value);
  Status ParseString(Stream& in, Value* value);
  Status ParseArray(Stream& in, Value* value);
  Status ParseObject(Stream& in, Value* value);

  // The root of the JSON document, default initialized to nullptr.
  Value value_;
};

// Macros to assign a value if the result of the rhs expression is not NULL.
// This is useful to read a known JSON structure into a native data format.

// Dereferences and assigns expression `rexpr` to `lhs` if `rexpr` is not null,
// or returns a util::Status otherwise. `rexpr` is only evaluated once. Note,
// this can only be called within a function returning util::Status.
#define JSON_ASSIGN_IFNN(lhs, rexpr) \
  JSON_ASSIGN_IFNN_IMPL(JSON_CONCAT(_tmp, __COUNTER__), lhs, rexpr)

// Dereferences and assigns expression `rexpr` to `lhs` if `rexpr` is not null,
// or assigns a default value otherwise. `rexpr` is only evaluated once.
#define JSON_ASSIGN_OR(lhs, rexpr, def) \
  JSON_ASSIGN_OR_IMPL(JSON_CONCAT(_tmp, __COUNTER__), lhs, rexpr, def)

#define JSON_ASSIGN_IFNN_IMPL(varname, lhs, rexpr)        \
  auto* varname = (rexpr);                                \
  if (varname == nullptr)                                 \
    return util::InvalidArgumentError("JSON NULL value"); \
  lhs = *varname;

#define JSON_ASSIGN_OR_IMPL(varname, lhs, rexpr, def)     \
  auto* varname = (rexpr);                                \
  lhs = (varname == nullptr) ? (def) : *varname

#define JSON_CONCAT_INNER(a, b) a##b
#define JSON_CONCAT(a, b) JSON_CONCAT_INNER(a, b)

// Inline implementation.

inline Json::Json(const Value& value) : value_(value) {}

inline const Json::Value& Json::GetRoot() const { return value_; }

template <typename T>
const T* Json::Value::Get() const {
  return std::get_if<T>(this);
}

inline const Json::Null* Json::Value::GetNull() const { return Get<Null>(); }

inline const Json::Bool* Json::Value::GetBool() const { return Get<Bool>(); }

inline const Json::Number* Json::Value::GetNumber() const {
  return Get<Number>();
}

inline const Json::String* Json::Value::GetString() const {
  return Get<String>();
}

inline const Json::Array* Json::Value::GetArray() const { return Get<Array>(); }

inline const Json::Object* Json::Value::GetObject() const {
  return Get<Object>();
}

}  // namespace util

#endif  // SRC_UTIL_JSON_H_
