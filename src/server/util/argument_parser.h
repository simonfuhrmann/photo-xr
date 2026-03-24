#ifndef SRC_SERVER_UTIL_ARGUMENT_PARSER_H_
#define SRC_SERVER_UTIL_ARGUMENT_PARSER_H_

#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace util {

// The parsed result with all positionals, options, and flags.
class ParsedArguments {
 public:
  // Returns the option's value, or the options default value if not provided.
  // Returns std::nullopt if the option was neither provided nor has a default.
  // GetOptionOrDie() prints an error and calls std::exit(1) if the option was
  // neither provided nor has a default.
  std::optional<std::string_view> GetOption(std::string_view long_name) const;
  std::string_view GetOptionOrDie(std::string_view long_name) const;

  // Returns the option's value as the specified type, or the options default
  // value if not provided. Prints and error and calls std::exit(1) if the
  // option was neither provided nor has a default, or if the value cannot be
  // converted to the specified type.
  int GetOptionAsIntOrDie(std::string_view long_name) const;
  double GetOptionAsDoubleOrDie(std::string_view long_name) const;

  // Returns true if the flag was provided.
  bool HasFlag(std::string_view long_name) const;

  // Returns positionals. The number of positionals is guaranteed to be between
  // the min and max in the spec. GetPositionalOrDie() returns the positional
  // or prints an error and calls std::exit(1) if the index is out of range.
  const std::vector<std::string>& GetPositionals() const;
  std::optional<std::string_view> GetPositional(size_t index) const;
  std::string_view GetPositionalOrDie(size_t index) const;

  std::map<std::string, std::string, std::less<>> options;
  std::set<std::string, std::less<>> flags;
  std::vector<std::string> positionals;
};

// A light-weight, feature complete, strict program argument parser. Arguments
// fall in three categories:
//
//   - Flags (boolean switches): -v --verbose --dry-run
//   - Options (with values): -o output.txt --input in.txt --output=out.txt
//   - Positional arguments: input.txt
//
// All flags and options must have a long name, and this is how they should be
// identified in the parsed result. Short names are optional. Duplicated short
// or long names are not allowed (Create() will return an error).
//
// A few notes on the parsing behavior:
//
//   - Repeated flags (-v -v) are equivalent to a single flag (-v).
//   - Repeated options (-o a.txt -o b.txt) are equivalent to the last option.
//   - Flags with values (--verbose=true) are treated as error.
//   - Empty options (--output=) are valid and produce empty string values.
//   - Bundling of short flags (-abc) is supported.
//   - Short options may omit the space before the value (-oout.txt).
//   - Long options can have an optional equals sign (--output=out.txt).
//   - Short options cannot have an equals sign (will be part of the value).
//   - Everything after "--" is treated as a positional argument.
//   - A lone dash (" - ") is treated as a positional with value "-".
//   - Empty positional arguments ("") are preserved.
//   - Any number of position arguments is allowed, and can be configured.
//   - The parser is strict, only specified flags and options are accepted.
//
// Currently, types are not supported, and everything is treated as a string.
// This leaves type-checks for option values to the caller.
class ArgumentParser {
 public:
  // Specification for a single option or flag.
  struct OptionSpec {
    std::string long_name;      // Required.
    char short_name = '\0';     // No short name if '\0'.
    bool has_value = false;     // Option if true, flag if false.
    std::string description;    // Used for help text.
    std::string default_value;  // Copied into result, ignored for flags.
  };

  // Specification for the argument parser.
  struct Spec {
    std::vector<OptionSpec> options;
    int min_positional_args = 0;      // Required number of positionals.
    int max_positional_args = -1;     // Any number allowed if negative.
    bool print_help_on_error = true;  // Print help text on parse error.
    std::string usage = "[options] [arg0 [arg1 [...]]]";
  };

  // Creates the ArgumentParser, or returns an error if the spec is invalid.
  // CreateOrDie() calls std::exit(1) instead of returning an error.
  static util::StatusOr<ArgumentParser> Create(const Spec& spec);
  static ArgumentParser CreateOrDie(const Spec& spec);

  // Parses arguments given in argc and argv. It treats argv[0] as the program
  // name, and argc MUST be at least 1. If parsing fails, an error is returned,
  // and the help text printed to stderr if `print_help_on_error` is true.
  // ParseOrDie() calls std::exit(1) instead of returning an error.
  util::StatusOr<ParsedArguments> Parse(int argc, const char* argv[]) const;
  ParsedArguments ParseOrDie(int argc, const char* argv[]) const;

  // Prints the help text to the given output stream. The help text is of the
  // following form, and requires the program name to be passed in:
  //
  //   Description of the program (Spec::program_description) goes here.
  //
  //   Usage: ./program [options] [arg0 [arg1 [...]]]
  //   Available options and flags:
  //     -o, --output ARG    Description of the output option.
  //     -v, --verbose       Description of the verbose flag.
  //     --dry-run           Description of flag without short name.
  void PrintHelpText(std::string_view argv0, std::ostream& os) const;

 private:
  ArgumentParser() = default;
  util::Status SetSpec(const Spec& spec);

  Spec spec_;
  std::map<std::string, size_t, std::less<>> long_lookup_;
  std::map<char, size_t> short_lookup_;
};

}  // namespace util

#endif  // SRC_SERVER_UTIL_ARGUMENT_PARSER_H_
