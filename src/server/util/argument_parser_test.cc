#include "src/server/util/argument_parser.h"

#include "src/server/test/tinytest.h"
#include "src/server/util/status_or.h"

namespace util {
namespace {

ArgumentParser::OptionSpec MakeSpec(std::string_view long_name, char short_name,
                                    bool has_value,
                                    std::string_view description,
                                    std::string_view default_value = "") {
  return {.long_name = std::string(long_name),
          .short_name = short_name,
          .has_value = has_value,
          .description = std::string(description),
          .default_value = std::string(default_value)};
}

#define MK_FLAG(ln, sh, descr, def) \
  MakeSpec(ln, sh, /*has_value=*/false, descr, def)
#define MK_OPTN(ln, sh, descr, def) \
  MakeSpec(ln, sh, /*has_value=*/true, descr, def)

}  // namespace

TEST(ArgumentParser, CreateValidation) {
  // Missing long name.
  {
    const ArgumentParser::Spec spec = {
        .options = {MK_FLAG("", 'v', "Verbose output", "")},
    };
    const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
    EXPECT_NOT_OK(parser);
    EXPECT_EQ(parser.status().message(), "Option long name cannot be empty");
  }

  // Flag with default value.
  {
    const ArgumentParser::Spec spec = {
        .options = {MK_FLAG("verbose", 'v', "Verbose output", "out.txt")},
    };
    const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
    EXPECT_NOT_OK(parser);
    EXPECT_EQ(parser.status().message(),
              "Flags cannot have default values: --verbose");
  }

  // Duplicated short name.
  {
    const ArgumentParser::Spec spec = {
        .options = {MK_OPTN("output", 'o', "Write results to file", ""),
                    MK_FLAG("verbose", 'o', "Output verbose logging", "")},
    };
    const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
    EXPECT_NOT_OK(parser);
    EXPECT_EQ(parser.status().message(), "Duplicate short option: -o");
  }

  // Duplicated long name.
  {
    const ArgumentParser::Spec spec = {
        .options = {MK_OPTN("output", 'o', "Write results to file", ""),
                    MK_FLAG("output", 'v', "Output verbose logging", "")},
    };
    const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
    EXPECT_NOT_OK(parser);
    EXPECT_EQ(parser.status().message(), "Duplicate long option: --output");
  }
}

TEST(ArgumentParser, PrintHelpText) {
  const ArgumentParser::Spec spec = {
      .options = {MK_FLAG("verbose", 'v', "Output verbose logging", ""),
                  MK_OPTN("output", 'o', "Write results to file", ""),
                  MK_FLAG("dry-run", '\0', "Run without making changes", "")},
  };

  util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
  ASSERT_OK(parser);
  std::stringstream ss;
  parser->PrintHelpText("./test_program", ss);
  EXPECT_EQ(ss.str(),
            "Usage: ./test_program [options] [arg0 [arg1 [...]]]\n"
            "Available options and flags:\n"
            "  -v, --verbose       Output verbose logging\n"
            "  -o, --output=ARG    Write results to file\n"
            "  --dry-run           Run without making changes\n");
}

TEST(ArgumentParser, ParseBasic) {
  const ArgumentParser::Spec spec = {
      .options = {MK_FLAG("verbose", 'v', "Output verbose logging", ""),
                  MK_OPTN("output", 'o', "Write results to file", ""),
                  MK_FLAG("dry-run", '\0', "Run without making changes", "")},
  };
  const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
  ASSERT_OK(parser);

  const char* argv[] = {"./tool", "-v", "--output=out.txt", "input.txt"};
  const util::StatusOr<ParsedArguments> result = parser->Parse(4, argv);
  ASSERT_OK(result);
  EXPECT_TRUE(result->HasFlag("verbose"));
  EXPECT_FALSE(result->HasFlag("dry-run"));
  EXPECT_EQ(result->GetOption("output"), "out.txt");
  EXPECT_EQ(result->GetOption("nonexistent"), std::nullopt);
  ASSERT_EQ(result->GetPositionals().size(), 1);
  EXPECT_EQ(result->GetPositionals()[0], "input.txt");
  EXPECT_EQ(result->GetPositional(0), "input.txt");
  EXPECT_EQ(result->GetPositional(1), std::nullopt);
}

TEST(ArgumentParser, ParseComplex) {
  const ArgumentParser::Spec spec = {
      .options = {MK_FLAG("aaa", 'a', "A description", ""),
                  MK_FLAG("bbb", 'b', "B description", ""),
                  MK_FLAG("ccc", 'c', "C description", ""),
                  MK_OPTN("ddd", 'd', "D description", ""),
                  MK_OPTN("eee", 'e', "E description", ""),
                  MK_OPTN("fff", 'f', "F description", ""),
                  MK_OPTN("ggg", 'g', "G description", ""),
                  MK_OPTN("hhh", 'h', "H description", "h-default"),
                  MK_OPTN("iii", 'i', "I description", "")},
  };
  const util::StatusOr<ArgumentParser> parser = ArgumentParser::Create(spec);
  ASSERT_OK(parser);

  const char* argv[] = {"./x", "-abd1", "-e", "2", "-f3", "--ggg=4", "pos0"};
  const util::StatusOr<ParsedArguments> result = parser->Parse(7, argv);
  ASSERT_OK(result);
  EXPECT_TRUE(result->HasFlag("aaa"));
  EXPECT_TRUE(result->HasFlag("bbb"));
  EXPECT_FALSE(result->HasFlag("ccc"));
  EXPECT_EQ(result->GetOption("ddd"), "1");
  EXPECT_EQ(result->GetOption("eee"), "2");
  EXPECT_EQ(result->GetOption("fff"), "3");
  EXPECT_EQ(result->GetOption("ggg"), "4");
  EXPECT_EQ(result->GetOption("hhh"), "h-default");
  EXPECT_EQ(result->GetOption("iii"), std::nullopt);
  ASSERT_EQ(result->GetPositionals().size(), 1);
  EXPECT_EQ(result->GetPositionals()[0], "pos0");
}

}  // namespace util
