#ifndef SRC_SERVER_API_HANDLER_H_
#define SRC_SERVER_API_HANDLER_H_

#include <map>

#include "src/server/net/http_handler.h"
#include "src/server/net/http_req_target.h"
#include "src/server/util/status.h"

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
//       { type: "photo", "name": "cover.jpg", "proj": "vr180" }
//     ]
//   }
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

 private:
  const Options options_;
};

}  // namespace server

#endif  // SRC_SERVER_API_HANDLER_H_
