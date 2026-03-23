#include "src/server/util/argument_parser.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace util {
namespace {
std::string GetOptionsHelpText(const ArgumentParser::OptionSpec& spec) {
  std::stringstream ss;
  if (spec.short_name != '\0') {
    ss << "-" << spec.short_name << ", ";
  }
  ss << "--" << spec.long_name;
  if (spec.has_value) ss << "=ARG";
  return ss.str();
}
}  // namespace

// Implementation for `ParsedArguments`.

std::optional<std::string_view> ParsedArguments::GetOption(
    std::string_view long_name) const {
  const auto iter = options.find(long_name);
  if (iter == options.end()) return std::nullopt;
  return iter->second;
}

bool ParsedArguments::HasFlag(std::string_view long_name) const {
  return flags.count(long_name) > 0;
}

const std::vector<std::string>& ParsedArguments::GetPositionals() const {
  return positionals;
}

std::optional<std::string_view> ParsedArguments::GetPositional(
    size_t index) const {
  if (index >= positionals.size()) return std::nullopt;
  return positionals[index];
}

// Implementation for `ArgumentParser`.

util::StatusOr<ArgumentParser> ArgumentParser::Create(const Spec& spec) {
  ArgumentParser parser;
  RETURN_IF_ERROR(parser.SetSpec(spec));
  return parser;
}

ArgumentParser ArgumentParser::CreateOrDie(const Spec& spec) {
  ArgumentParser parser;
  if (const util::Status status = parser.SetSpec(spec); !status.ok()) {
    std::cerr << "Invalid argument parser spec: " << status.message() << "\n";
    std::abort();
  }
  return parser;
}

util::Status ArgumentParser::SetSpec(const Spec& spec) {
  spec_ = spec;
  long_lookup_.clear();
  short_lookup_.clear();

  if (spec.max_positional_args >= 0 &&
      spec.max_positional_args < spec.min_positional_args) {
    return util::InvalidArgumentError("Invalid positional argument spec");
  }

  for (size_t i = 0; i < spec_.options.size(); ++i) {
    const OptionSpec& option = spec_.options[i];
    // Flags must not have a default value.
    if (!option.has_value && !option.default_value.empty()) {
      return util::InvalidArgumentError("Flags cannot have default values: --" +
                                        option.long_name);
    }

    // All flags and options must have a long name.
    if (option.long_name.empty()) {
      return util::InvalidArgumentError("Option long name cannot be empty");
    }
    // Reject duplicate long names.
    if (long_lookup_.count(option.long_name) > 0) {
      return util::InvalidArgumentError("Duplicate long option: --" +
                                        option.long_name);
    }
    long_lookup_[option.long_name] = i;

    // Short option name '-' is not allowed.
    if (option.short_name == '-') {
      return util::InvalidArgumentError("Invalid short option name: -");
    }

    // Short names for flags and options are optional.
    if (option.short_name != '\0') {
      // Reject duplicate short names.
      if (short_lookup_.count(option.short_name) > 0) {
        return util::InvalidArgumentError("Duplicate short option: -" +
                                          std::string(1, option.short_name));
      }
      short_lookup_[option.short_name] = i;
    }
  }
  return util::OkStatus();
}

util::StatusOr<ParsedArguments> ArgumentParser::Parse(
    int argc, const char* argv[]) const {
  // If no arguments are given at all, return error. At least the program name
  // is expected to be provided.
  if (argc <= 0) {
    return util::InvalidArgumentError("No arguments provided");
  }

  auto error = [&](const std::string& msg) -> util::Status {
    if (spec_.print_help_on_error) {
      std::cerr << "\n";
      PrintHelpText(argv[0], std::cerr);
      std::cerr << "\nError: " << msg << "\n";
    }
    return util::InvalidArgumentError(msg);
  };

  ParsedArguments result;
  bool parsing_positionals_only = false;  // True after "--" is seen.
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // Handle "--" terminator.
    if (!parsing_positionals_only && arg == "--") {
      parsing_positionals_only = true;
      continue;
    }

    // Handle positional arguments.
    if (parsing_positionals_only || arg.empty() || arg == "-" ||
        arg[0] != '-') {
      result.positionals.push_back(arg);
      continue;
    }

    // Long option or flag: --foo=bar or --foo
    if (arg.compare(0, 2, "--") == 0) {
      std::string name;
      std::optional<std::string> value;
      const size_t eq = arg.find('=');
      if (eq == std::string::npos) {
        name = arg.substr(2);
      } else {
        name = arg.substr(2, eq - 2);
        value = arg.substr(eq + 1);
      }

      if (name.empty()) {
        return error("Invalid option format: " + arg);
      }

      const auto it = long_lookup_.find(name);
      if (it == long_lookup_.end()) {
        return error("Unknown option: --" + name);
      }

      const OptionSpec& spec = spec_.options[it->second];
      if (spec.has_value) {
        // Long options without '=' must have value in next argv.
        if (!value.has_value()) {
          if (i + 1 >= argc) {
            return error("Missing value for option: --" + name);
          }
          value = argv[++i];
        }
        result.options[spec.long_name] = *value;
      } else {
        // Flags that have a value set via '=' are not allowed.
        if (value.has_value()) {
          return error("Flag does not take a value: --" + name);
        }
        result.flags.insert(spec.long_name);
      }
      continue;
    }

    // Short options: -a or -abc
    if (arg[0] == '-') {
      // Lone "-" already handled earlier, but handle for clarity.
      if (arg.size() == 1) {
        result.positionals.push_back(arg);
        continue;
      }

      // Iterate bundled short flags.
      for (size_t j = 1; j < arg.size(); ++j) {
        const char c = arg[j];
        const auto it = short_lookup_.find(c);
        if (it == short_lookup_.end()) {
          return error(std::string("Unknown short option: -") + c);
        }

        const OptionSpec& spec = spec_.options[it->second];
        if (!spec.has_value) {
          // The option is a flag.
          result.flags.insert(spec.long_name);
          continue;
        }

        // If the option takes value, take the rest of the bundle as value, or
        // take the next argv as value if the bundle ends here.
        std::string value;
        if (j + 1 < arg.size()) {
          value = arg.substr(j + 1);
        } else if (i + 1 < argc) {
          value = argv[++i];
        } else {
          return error(std::string("Missing value for option: -") + c);
        }
        result.options[spec.long_name] = value;
        break;
      }
    }
  }

  // Apply defaults for options not provided.
  for (const OptionSpec& spec : spec_.options) {
    if (!spec.has_value || spec.default_value.empty()) continue;
    if (result.options.count(spec.long_name) > 0) continue;
    result.options[spec.long_name] = spec.default_value;
  }

  // Validate positional count.
  const int count = static_cast<int>(result.positionals.size());
  if (count < spec_.min_positional_args) {
    return error("Too few positional arguments");
  }
  if (spec_.max_positional_args >= 0 && count > spec_.max_positional_args) {
    return error("Too many positional arguments");
  }

  return result;
}

void ArgumentParser::PrintHelpText(std::string_view argv0,
                                   std::ostream& os) const {
  os << "Usage: " << argv0 << " [options] [arg0 [arg1 [...]]]\n";
  if (spec_.options.empty()) return;

  os << "Available options and flags:\n";
  for (const auto& option : spec_.options) {
    os << "  " << std::setw(20) << std::left << GetOptionsHelpText(option)
       << option.description << "\n";
  }
}

}  // namespace util
