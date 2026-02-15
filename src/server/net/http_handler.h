#ifndef SRC_NET_HTTP_HANDLER_H_
#define SRC_NET_HTTP_HANDLER_H_

#include <string>

#include "src/server/net/http_request.h"
#include "src/server/net/http_types.h"
#include "src/server/util/status.h"

namespace net {

// Handler abstract base class. If the `Handle()` function returns OK status,
// or if `request` was replied to, then `HttpServer` considers the request
// handled and stops processing the request.
class HttpHandlerBase {
 public:
  HttpHandlerBase() = default;
  virtual ~HttpHandlerBase() = default;
  virtual util::Status Handle(HttpRequest& request) const = 0;

  // Disallow copy and assign.
  HttpHandlerBase(const HttpHandlerBase&) = delete;
  HttpHandlerBase& operator=(const HttpHandlerBase&) = delete;
};

// Handler for a static reply. This is usually registered as the last handler
// to serve a 404 error page.
class HttpStaticPageHandler : public HttpHandlerBase {
 public:
  struct Options {
    HttpStatus status_code = HttpStatus::CODE_404_NOT_FOUND;
    std::string content_type = "text/plain";
    std::string page_body = "File not found";
  };

  explicit HttpStaticPageHandler(const Options& options);
  util::Status Handle(HttpRequest& request) const override;

 private:
  const Options options_;
};

// Handler for static files. Only GET requests with an "origin-form" (i.e.,
// request paths that start with a "/") are supported and interpreted as file
// requests relative to the root path. For security considerations, request
// paths are always normalized (dot and dot-dot components are removed) to
// prevent directory traversal attacks, e.g., "GET /../../etc/passwd HTTP/1.1".
//
// Note: Requests to "/" are rewritten as "/index.html". Maybe this should be
// configurable. Directory listings are not supported.
//
// Note: File contents are loaded into memory in their entirety before being
// served to the client, and memory-efficient streaming is not supported.
class HttpStaticFileHandler : public HttpHandlerBase {
 public:
  struct Options {
    // The root directory that is joined with the request path to locate the
    // file in the file system.
    std::string root_dir;

    // Ensures that the canonicalized root directory is a prefix of the
    // canonicalized root directory joined with the request path. This ensures
    // that request never leave the root path. This prevents serving symlinks
    // under the root directory that point outside the root directory.
    bool strict_root = true;

    // A path prefix that is required on the request path to be matched with
    // this handler, and that is stripped from the request path before joining
    // with the root directory. This is useful for serving the `root_dir` under
    // a specific path prefix. Example: "/photos" (no trailing slash).
    std::string path_prefix;
  };

  explicit HttpStaticFileHandler(const Options& options);
  util::Status Handle(HttpRequest& request) const override;

 private:
  const Options options_;
};

}  // namespace net

#endif  // SRC_NET_HTTP_HANDLER_H_
