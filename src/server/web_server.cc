#include "src/server/web_server.h"

#include <csignal>
#include <iostream>

#include "src/server/album_cache.h"

namespace server {
namespace {

net::HttpServer::Options GetHttpServerOptions(
    const WebServer::Options& options) {
  net::HttpServer::Options http_options;
  http_options.listen_address = options.listen_address;
  http_options.listen_port = options.listen_port;
  http_options.num_threads = options.num_threads;
  return http_options;
}

std::string_view GetAlbumCacheOptions(const WebServer::Options& options) {
  return options.media_root;
}

ApiHandler::Options GetApiHandlerOptions(const AlbumCache& album_cache) {
  ApiHandler::Options api_options;
  api_options.album_cache = &album_cache;
  api_options.reply_headers["Access-Control-Allow-Origin"] = "*";
  api_options.reply_headers["X-Content-Type-Options"] = "nosniff";
  return api_options;
}

MediaHandler::Options GetMediaHandlerOptions(const AlbumCache& album_cache) {
  MediaHandler::Options media_options;
  media_options.album_cache = &album_cache;
  media_options.reply_headers["Cache-Control"] = "public, max-age=3600";
  media_options.reply_headers["Access-Control-Allow-Origin"] = "*";
  media_options.reply_headers["X-Content-Type-Options"] = "nosniff";
  return media_options;
}

net::HttpStaticFileHandler::Options GetFileHandlerOptions() {
  net::HttpStaticFileHandler::Options options;
  options.root_dir = "src/client/";
  options.strict_root = false;  // Bazel output are symlinks.
  return options;
}

net::HttpStaticPageHandler::Options GetErrorHandlerOptions() {
  return net::HttpStaticPageHandler::Options();
}

}  // namespace

WebServer::WebServer(const Options& options)
    : http_server_(GetHttpServerOptions(options)),
      album_cache_(GetAlbumCacheOptions(options)),
      api_handler_(GetApiHandlerOptions(album_cache_)),
      media_handler_(GetMediaHandlerOptions(album_cache_)),
      file_handler_(GetFileHandlerOptions()),
      error_handler_(GetErrorHandlerOptions()) {
  http_server_.RegisterHandler(&logger_handler_);
  http_server_.RegisterHandler(&api_handler_);
  http_server_.RegisterHandler(&media_handler_);
  http_server_.RegisterHandler(&file_handler_);
  http_server_.RegisterHandler(&error_handler_);
  signal_handler_.SetCallback(SIGINT, [this]() { Stop().IgnoreError(); });
}

WebServer::~WebServer() { Stop().IgnoreError(); }

util::Status WebServer::Start() {
  const net::HttpServer::Options& http_opts = http_server_.GetOptions();
  std::cout << "Starting HTTP server on " << http_opts.listen_address << ":"
            << http_opts.listen_port << "..." << std::endl;
  return http_server_.Start();
}

util::Status WebServer::Stop() {
  if (http_server_.IsRunning()) {
    std::cout << "Stopping HTTP server..." << std::endl;
    RETURN_IF_ERROR(http_server_.Stop());
  }
  return util::OkStatus();
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
