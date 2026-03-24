#ifndef SRC_SERVER_MEDIA_HANDLER_H_
#define SRC_SERVER_MEDIA_HANDLER_H_

#include <map>
#include <string>

#include "src/server/net/http_handler.h"
#include "src/server/net/http_request.h"
#include "src/server/util/status.h"

namespace server {

// Serves photo and video media files under the "/media" prefix.
//
// The renderer in the client expects side-by-side 180 degrees equirect media.
// If the photo or video is in SBS format, it will be served as-is. For VR180
// photos, since the client does not have easy access to the XMP metadata,
// the handler transparently stitches these photos to 180 degree SBS.
class MediaHandler : public net::HttpStaticFileHandler {
 public:
  struct Options {
    std::string media_root;
    std::map<std::string, std::string> reply_headers;
  };

  explicit MediaHandler(const Options& options);
  util::Status Handle(net::HttpRequest& request) const override;

 private:
  const Options options_;
};

}  // namespace server

#endif  // SRC_SERVER_MEDIA_HANDLER_H_
