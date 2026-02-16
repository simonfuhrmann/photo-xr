#ifndef SRC_SERVER_API_HANDLER_H_
#define SRC_SERVER_API_HANDLER_H_

#include <map>
#include <string>
#include <string_view>

#include "src/server/net/http_handler.h"
#include "src/server/net/http_req_target.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace server {

// Handles API requests. The app currently supports API requests of form:
//
//   GET /api/album?path=
//   GET /api/album?path=Travel
//   GET /api/album?path=Travel/Iceland
//
// At each directory level, the API responds with a JSON of the directory
// contents, which are either sub-albums, or photos with metadata. Example:
//
//   {
//     "path": "Travel",
//     "entries": [
//       { type: "album", "name": "Iceland" },
//       { type: "album", "name": "Japan" },
//       { type: "photo", "name": "cover.jpg", "eyes": "separate" }
//     ]
//   }
//
// Currently supported layouts for photos are
//
// - "separate": Google's VR photo format, which has the left eye in the JPEG
//   data, the right eye in the XMP metadata as base64-encoded JPEG.
//   https://developers.google.com/vr/reference/cardboard-camera-vr-photo-format
// - "sbs": Side-by-side format, where the left and right eyes are stored in the
//   same JPEG image, left and right half are the cooresponding eye.
//
// The renderer in the client can map each side of an SBS image to a texture
// for rendering. However, for separate images, the client does not have easy
// access to the XMP metadata; thus the API offers requests to get the left and
// right eye images separately.
//
//   GET /api/photo?path=Travel/Iceland/cover.jpg&eye=left
//   GET /api/photo?path=Travel/Iceland/cover.jpg&eye=right
//
// This API request will fail if the JPEG does not contain embedded XMP data.
class ApiHandler : public net::HttpHandlerBase {
 public:
  struct Options {
    std::string photos_root;
    std::map<std::string, std::string> reply_headers;
  };

  ApiHandler(const Options& options);

 protected:
  util::Status Handle(net::HttpRequest& request) const override;
  util::Status HandleAlbumRequest(net::HttpRequest& request,
                                  const net::HttpReqTarget& target) const;
  util::Status HandlePhotoRequest(net::HttpRequest& request,
                                  const net::HttpReqTarget& target) const;
  util::Status HandlePhotoLeftEyeRequest(net::HttpRequest& request,
                                         std::string_view local_path) const;
  util::Status HandlePhotoRightEyeRequest(net::HttpRequest& request,
                                          std::string_view local_path) const;

 private:
  util::StatusOr<std::string> GetLocalPath(std::string_view req_path) const;

  const Options options_;
};

}  // namespace server

#endif  // SRC_SERVER_API_HANDLER_H_
