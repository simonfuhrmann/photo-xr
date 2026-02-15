#ifndef SRC_SERVER_WEB_SERVER_H_
#define SRC_SERVER_WEB_SERVER_H_

#include "src/server/net/http_handler.h"
#include "src/server/net/http_server.h"
#include "src/server/util/status.h"

namespace server {

class WebServer {
 public:
  struct Options {
    int listen_port = 8080;
    int num_threads = 4;
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
  LoggerHandler logger_handler_;
  net::HttpStaticFileHandler file_handler_;
  net::HttpStaticPageHandler error_handler_;
};

}  // namespace server

#endif  // SRC_SERVER_WEB_SERVER_H_
