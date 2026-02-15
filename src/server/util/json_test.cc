#include "src/server/util/json.h"

#include "src/server/test/tinytest.h"
#include "src/server/util/status.h"

namespace util {

TEST(Json, ParseNull) {
  Json json;
  const Json::Value& root = json.GetRoot();

  Json::Status status = json.Parse(R"(null)");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetNull(), nullptr);
  EXPECT_EQ(*root.GetNull(), nullptr);

  // Trailing characters at the end.
  status = json.Parse(R"(nullx)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 4);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, ParseBoolean) {
  Json json;
  const Json::Value& root = json.GetRoot();

  // A true value.
  Json::Status status = json.Parse(R"(true)");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetBool(), nullptr);
  EXPECT_EQ(*root.GetBool(), true);

  // A false value.
  status = json.Parse(R"(false)");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetBool(), nullptr);
  EXPECT_EQ(*root.GetBool(), false);

  // An invalid value.
  status = json.Parse(R"(tool)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 0);
  EXPECT_EQ(status.error_msg, "Unexpected token");

  // Trailing characters at the end.
  status = json.Parse(R"(truex)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 4);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, ParseNumber) {
  Json json;
  const Json::Value& root = json.GetRoot();

  // An integer value.
  Json::Status status = json.Parse(R"(123)");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetNumber(), nullptr);
  EXPECT_EQ(*root.GetNumber(), 123);

  // A negative double.
  status = json.Parse(R"(-1.23)");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetNumber(), nullptr);
  EXPECT_EQ(*root.GetNumber(), -1.23);

  // Invalid number.
  status = json.Parse(R"(1-23)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 0);
  EXPECT_EQ(status.error_msg, "Number parse error");

  // Trailing characters at the end.
  status = json.Parse(R"(1.234x)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 5);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, ParseString) {
  Json json;
  const Json::Value& root = json.GetRoot();

  // A totally ordinary string.
  Json::Status status = json.Parse(R"("test")");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetString(), nullptr);
  EXPECT_EQ(*root.GetString(), "test");

  // Escaped quote.
  status = json.Parse(R"("te\"st")");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetString(), nullptr);
  EXPECT_EQ(*root.GetString(), "te\"st");

  // Missing end quote causes unexpected EOF.
  status = json.Parse(R"("test)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 5);
  EXPECT_EQ(status.error_msg, "Unexpected EOF");

  // Trailing characters at the end.
  status = json.Parse(R"("test"x)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 6);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, ParseArray) {
  Json json;
  const Json::Value& root = json.GetRoot();

  // Test with an empty array.
  Json::Status status = json.Parse(R"([])");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetArray(), nullptr);
  EXPECT_TRUE(root.GetArray()->empty());

  // Test with various data types.
  status = json.Parse(R"([true, "test", 10])");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetArray(), nullptr);
  const Json::Array& array = *root.GetArray();
  ASSERT_EQ(array.size(), 3);
  EXPECT_NE(array[0].GetBool(), nullptr);
  EXPECT_EQ(*array[0].GetBool(), true);
  EXPECT_NE(array[1].GetString(), nullptr);
  EXPECT_EQ(*array[1].GetString(), "test");
  EXPECT_NE(array[2].GetNumber(), nullptr);
  EXPECT_EQ(*array[2].GetNumber(), 10.0);

  // Test failure on trailing comma.
  status = json.Parse(R"([true, ])");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 7);
  EXPECT_EQ(status.error_msg, "Unexpected character: ]");

  // Test missing comma.
  status = json.Parse(R"([1 2])");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 3);
  EXPECT_EQ(status.error_msg, "Invalid array separator");

  // Test trailing garbage.
  status = json.Parse(R"([1, 2]x)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 6);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, ParseObject) {
  Json json;
  const Json::Value& root = json.GetRoot();

  // Test with an empty object.
  Json::Status status = json.Parse(R"({})");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetObject(), nullptr);
  EXPECT_TRUE(root.GetObject()->empty());

  // Test with various data types.
  status = json.Parse(R"(
    {
      "name": "Alice",
      "age": 30,
      "married": true,
      "children": ["Bob", "Charlie"],
      "meta": null
    }
  )");
  ASSERT_TRUE(status.success);
  ASSERT_NE(root.GetObject(), nullptr);
  const Json::String* name_value = root.GetObjectString("name");
  ASSERT_NE(name_value, nullptr);
  EXPECT_EQ(*name_value, "Alice");
  const Json::Number* age_value = root.GetObjectNumber("age");
  ASSERT_NE(age_value, nullptr);
  EXPECT_EQ(*age_value, 30.0);
  const Json::Bool* married_value = root.GetObjectBool("married");
  ASSERT_NE(married_value, nullptr);
  EXPECT_EQ(*married_value, true);
  const Json::Array* children_value = root.GetObjectArray("children");
  ASSERT_NE(children_value, nullptr);
  ASSERT_EQ(children_value->size(), 2);
  ASSERT_NE(children_value->at(0).GetString(), nullptr);
  EXPECT_EQ(*children_value->at(0).GetString(), "Bob");
  ASSERT_NE(children_value->at(1).GetString(), nullptr);
  EXPECT_EQ(*children_value->at(1).GetString(), "Charlie");
  const Json::Null* meta_value = root.GetObjectNull("meta");
  ASSERT_NE(meta_value, nullptr);
  EXPECT_EQ(*meta_value, nullptr);

  // Test early EOF.
  status = json.Parse(R"({"name": "Alice",)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 16);
  EXPECT_EQ(status.error_msg, "Unexpected EOF");

  // Test failure on trailing comma.
  status = json.Parse(R"({"key": "value", })");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 17);
  EXPECT_EQ(status.error_msg, "Invalid object key");

  // Test trailing garbage.
  status = json.Parse(R"({}x)");
  ASSERT_FALSE(status.success);
  EXPECT_EQ(status.error_row, 0);
  EXPECT_EQ(status.error_col, 2);
  EXPECT_EQ(status.error_msg, "Trailing characters");
}

TEST(Json, Serialize) {
  EXPECT_EQ(Json::Serialize(nullptr), "null");
  EXPECT_EQ(Json::Serialize(true), "true");
  EXPECT_EQ(Json::Serialize(false), "false");
  EXPECT_EQ(Json::Serialize(10.0), "10");
  EXPECT_EQ(Json::Serialize("te\tst"), "\"te\\tst\"");

  Json json;
  const Json::Status status = json.Parse(R"(
    {
      "name": "Alice",
      "age": 30,
      "married": true,
      "children": ["Bob", "Charlie"],
      "meta": null
    }
  )");
  ASSERT_TRUE(status.success);

  std::string str = json.Serialize();
  EXPECT_EQ(str, R"({
  "age": 30,
  "children": [
    "Bob",
    "Charlie"
  ],
  "married": true,
  "meta": null,
  "name": "Alice"
})");
}

util::Status TryReadString(const Json::Value& json_str, std::string* out) {
  JSON_ASSIGN_IFNN(*out, json_str.GetString());
  return util::OkStatus();
}

TEST(JSON, Macros) {
  const Json::Value number_val = Json::Number(10);
  const Json::Value string_val = Json::String("test");

  // Test for JSON_ASSIGN_IFNN.
  std::string str;
  ASSERT_NOT_OK(TryReadString(number_val, &str));
  ASSERT_OK(TryReadString(string_val, &str));
  EXPECT_EQ(str, "test");

  // Test for JSON_ASSIGN_OR.
  JSON_ASSIGN_OR(const int number1, number_val.GetNumber(), 42);
  JSON_ASSIGN_OR(const int number2, string_val.GetNumber(), 42);
  EXPECT_EQ(number1, 10);
  EXPECT_EQ(number2, 42);
}

}  // namespace util
