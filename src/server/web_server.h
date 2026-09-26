#ifndef SRC_SERVER_WEB_SERVER_H_
#define SRC_SERVER_WEB_SERVER_H_

#include "src/server/album_cache.h"
#include "src/server/api_handler.h"
#include "src/server/media_handler.h"
#include "src/server/net/http_handler.h"
#include "src/server/net/http_server.h"
#include "src/server/signal_handler.h"
#include "src/server/util/status.h"

namespace server {

class WebServer {
 public:
  struct Options {
    std::string listen_address = "127.0.0.1";
    int listen_port = 8080;
    int num_threads = 4;
    std::string media_root;
  };

  WebServer(const Options& options);
  ~WebServer();
  util::Status Start();
  util::Status Stop();

 private:
  // A hander that logs all HTTP requests.
  class LoggerHandler : public net::HttpHandlerBase {
    util::Status Handle(net::HttpRequest& request) const override;
  };

  net::HttpServer http_server_;
  AlbumCache album_cache_;
  LoggerHandler logger_handler_;
  ApiHandler api_handler_;
  MediaHandler media_handler_;
  net::HttpStaticFileHandler file_handler_;
  net::HttpStaticPageHandler error_handler_;
  SignalHandler signal_handler_;
};

}  // namespace server

#endif  // SRC_SERVER_WEB_SERVER_H_
