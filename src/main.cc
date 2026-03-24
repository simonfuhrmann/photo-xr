#include <cstdlib>
#include <iostream>

#include "src/server/util/argument_parser.h"
#include "src/server/util/status.h"
#include "src/server/web_server.h"

int main(int argc, const char** argv) {
  const util::ArgumentParser::Spec spec = {
      .options = {{.long_name = "server-port",
                   .has_value = true,
                   .description = "Server port to listen on",
                   .default_value = "8080"}},
      .min_positional_args = 1,
      .max_positional_args = 1,
      .print_help_on_error = true,
      .usage = "[options] <media_root>",
  };
  const auto arg_parser = util::ArgumentParser::CreateOrDie(spec);
  const util::ParsedArguments args = arg_parser.ParseOrDie(argc, argv);

  server::WebServer::Options options;
  options.listen_port = args.GetOptionAsIntOrDie("server-port");
  options.media_root = args.GetPositionalOrDie(0);
  options.num_threads = 4;
  server::WebServer web_server(options);
  const util::Status status = web_server.Start();
  if (!status.ok()) {
    std::cerr << "Failed starting web server: " << status.message() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
