#include "src/server/web_server.h"

#include <iostream>

namespace server {
namespace {

net::HttpServer::Options GetHttpServerOptions(
    const WebServer::Options& options) {
  net::HttpServer::Options http_options;
  http_options.listen_address = "127.0.0.1";
  http_options.listen_port = options.listen_port;
  http_options.num_threads = options.num_threads;
  return http_options;
}

ApiHandler::Options GetApiHandlerOptions(const WebServer::Options& options) {
  ApiHandler::Options api_options;
  api_options.photos_root = options.photos_root;
  api_options.reply_headers["Access-Control-Allow-Origin"] = "*";
  api_options.reply_headers["X-Content-Type-Options"] = "nosniff";
  return api_options;
}

net::HttpStaticPageHandler::Options GetErrorHandlerOptions() {
  return net::HttpStaticPageHandler::Options();
}

net::HttpStaticFileHandler::Options GetPhotoHandlerOptions(
    const WebServer::Options& options) {
  net::HttpStaticFileHandler::Options photo_options;
  photo_options.root_dir = options.photos_root;
  photo_options.path_prefix = "/photo";
  photo_options.reply_headers["Cache-Control"] = "public, max-age=3600";
  photo_options.reply_headers["Access-Control-Allow-Origin"] = "*";
  photo_options.reply_headers["X-Content-Type-Options"] = "nosniff";
  return photo_options;
}

net::HttpStaticFileHandler::Options GetFileHandlerOptions() {
  net::HttpStaticFileHandler::Options options;
  options.root_dir = "src/client/";
  options.strict_root = false;  // Bazel output are symlinks.
  return options;
}

}  // namespace

WebServer::WebServer(const Options& options)
    : http_server_(GetHttpServerOptions(options)),
      api_handler_(GetApiHandlerOptions(options)),
      photo_handler_(GetPhotoHandlerOptions(options)),
      file_handler_(GetFileHandlerOptions()),
      error_handler_(GetErrorHandlerOptions()) {
  http_server_.RegisterHandler(&logger_handler_);
  http_server_.RegisterHandler(&api_handler_);
  http_server_.RegisterHandler(&photo_handler_);
  http_server_.RegisterHandler(&file_handler_);
  http_server_.RegisterHandler(&error_handler_);
}

WebServer::~WebServer() { Stop().IgnoreError(); }

util::Status WebServer::Start() {
  const net::HttpServer::Options& http_opts = http_server_.GetOptions();
  std::cout << "Starting HTTP server on " << http_opts.listen_address << ":"
            << http_opts.listen_port << "..." << std::endl;
  const util::Status status = http_server_.Start();
  if (!status.ok()) {
    std::cerr << "Failed starting HTTP server: " << status.message()
              << std::endl;
  }
  return status;
}

util::Status WebServer::Stop() {
  if (http_server_.IsRunning()) {
    std::cout << "Stopping HTTP server..." << std::endl;
    RETURN_IF_ERROR(http_server_.Stop());
  }
  return util::Status();
}

util::Status WebServer::LoggerHandler::Handle(net::HttpRequest& request) const {
  std::cout << "HTTP request: " << request.GetRequestLine();
  if (request.GetRequestMethod() == net::HttpMethod::POST) {
    std::cout << " (" << request.GetRequestBody().size() << " bytes)";
  }
  std::cout << std::endl;
  return util::AbortedError("");
}

}  // namespace server
