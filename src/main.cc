#include <cstdlib>
#include <iostream>

#include "src/server/util/status.h"
#include "src/server/web_server.h"

int main(int argc, char** argv) {
  server::WebServer::Options options;
  options.listen_port = 8080;
  options.num_threads = 4;
  options.photos_root = "/data/pics/vr_photos/";
  server::WebServer web_server(options);
  const util::Status status = web_server.Start();
  if (!status.ok()) {
    std::cerr << "Failed starting web server: " << status.message() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
