#include "src/server/net/http_handler.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <string_view>

#include "src/server/net/http_range_request.h"
#include "src/server/net/http_request.h"
#include "src/server/net/http_types.h"
#include "src/server/util/file_utils.h"
#include "src/server/util/status_or.h"
#include "src/server/util/string_utils.h"

namespace net {
namespace {

// Returns the filename that results from concatenating the root path with the
// request path. The returned filename is canonicalized and may not contain the
// root path as prefix if the requested file traverses a symlink.
util::StatusOr<std::string> GetLocalRequestPath(
    std::string_view http_request_target,
    const HttpStaticFileHandler::Options& options) {
  // Only paths starting with "/" are supported.
  if (http_request_target.empty() || http_request_target.front() != '/') {
    return util::InvalidArgumentError("Invalid request path");
  }

  // Strip query and fragment components from the path.
  const size_t query_pos = http_request_target.find_first_of('?');
  const size_t frag_pos = http_request_target.find_first_of('#');
  const size_t strip_pos = std::min(query_pos, frag_pos);
  std::string_view req_path = http_request_target.substr(0, strip_pos);

  // If the request path is "/", rewrite it as "/index.html".
  if (req_path == "/" && !options.root_rewrite.empty()) {
    req_path = options.root_rewrite;
  }

  // Normalize request path, concatenate with root dir, and canonicalize.
  return util::GetCanonicalPath(
      util::StrCat(options.root_dir, "/", util::GetNormalizedPath(req_path)));
}

// Returns OK only if `root_path` is a prefix of `req_path`.
util::Status EnsureRootPathPrefix(std::string_view req_path,
                                  std::string_view root_path) {
  return util::HasPrefix(req_path, root_path)
             ? util::OkStatus()
             : util::FailedPreconditionError("Request path is not rooted");
}

// Returns a mime type, by finding `filename`s extension in a mapping from
// extension to common mime types. This should perhaps be configurable.
// Initialization (the first call) is not thread safe also not thread safe, and
// should perhaps use std::call_once.
std::string_view GetContentType(std::string_view filename) {
  constexpr std::string_view kMimeTypeFallback = "application/octet-stream";
  static std::map<std::string_view, std::string_view> ext_map;
  if (ext_map.empty()) {
    ext_map = std::map<std::string_view, std::string_view>{
        {"aac", "audio/aac"},
        {"apng", "image/apng"},
        {"avi", "video/x-msvideo"},
        {"bmp", "image/bmp"},
        {"bz", "application/x-bzip"},
        {"bz2", "application/x-bzip2"},
        {"css", "text/css"},
        {"csv", "text/csv"},
        {"gz", "application/gzip"},
        {"gif", "image/gif"},
        {"htm", "text/html"},
        {"html", "text/html"},
        {"ico", "image/vnd.microsoft.icon"},
        {"ics", "text/calendar"},
        {"jpeg", "image/jpeg"},
        {"jpg", "image/jpeg"},
        {"js", "text/javascript"},
        {"mjs", "text/javascript"},
        {"json", "application/json"},
        {"md", "text/markdown"},
        {"mid", "audio/midi"},
        {"midi", "audio/midi"},
        {"mp3", "audio/mpeg"},
        {"mp4", "video/mp4"},
        {"mpeg", "video/mpeg"},
        {"png", "image/png"},
        {"pdf", "application/pdf"},
        {"svg", "image/svg+xml"},
        {"tar", "application/x-tar"},
        {"tif", "image/tiff"},
        {"tiff", "image/tiff"},
        {"ttf", "font/ttf"},
        {"txt", "text/plain"},
        {"wav", "audio/wav"},
        {"weba", "audio/webm"},
        {"webm", "video/webm"},
        {"webp", "image/webp"},
        {"xhtml", "application/xhtml+xml"},
        {"xml", "application/xml"},
        {"zip", "application/zip"},
    };
  }

  std::string_view ext = util::GetFileExtension(filename);
  const auto iter = ext_map.find(ext);
  if (iter == ext_map.end()) return kMimeTypeFallback;
  return iter->second;
}

}  // namespace

HttpStaticPageHandler::HttpStaticPageHandler(const Options& options)
    : options_(options) {}

util::Status HttpStaticPageHandler::Handle(HttpRequest& request) const {
  request.SetReplyStatus(options_.status_code);
  request.SetReplyContentType(options_.content_type);
  request.SetReplyBody(options_.page_body, /*copy_data=*/false);
  return request.Reply();
}

HttpStaticFileHandler::HttpStaticFileHandler(const Options& options)
    : options_(options) {}

util::Status HttpStaticFileHandler::Handle(HttpRequest& request) const {
  // Only GET requests are supported.
  if (request.GetRequestMethod() != HttpMethod::GET) {
    return util::InvalidArgumentError("Only GET requests are supported");
  }

  // Ensure the `path_prefix` is present and strip it from the request path.
  std::string_view req_path = request.GetRequestTarget();
  if (!util::HasPrefix(req_path, options_.path_prefix)) {
    return util::NotFoundError("File not found");
  }
  req_path.remove_prefix(options_.path_prefix.size());

  // Get the concatenation of the root directory and the normalized request
  // path, with URI query and fragment components removed, then canonicalized.
  // This is an existing file in the local file system.
  ASSIGN_OR_RETURN(const std::string& local_path,
                   GetLocalRequestPath(req_path, options_));

  // With `strict_root` enabled, make sure the canonical root directory is a
  // prefix of the request path.
  if (options_.strict_root) {
    ASSIGN_OR_RETURN(const std::string root_dir,
                     util::GetCanonicalPath(options_.root_dir));
    RETURN_IF_ERROR(EnsureRootPathPrefix(local_path, root_dir));
  }

  // Try to handle the request as a range request. If the request was handled,
  // true is returned, and this function is done. Otherwise, treat the request
  // as a full file request.
  ASSIGN_OR_RETURN(const bool range_request_handled,
                   MaybeHandleRangeRequest(request, local_path));
  if (range_request_handled) return util::OkStatus();

  // Treat the request as a full file request. Open file and determine size.
  std::ifstream input(local_path.data(), std::ios::binary);
  if (input.fail()) {
    return util::InternalError("Failed to open file");
  }
  input.seekg(0, std::ios::end);
  const size_t file_size = input.tellg();
  input.seekg(0, std::ios::beg);
  if (input.fail()) {
    return util::InternalError("Failed to determine file size");
  }

  // Set HTTP status and headers, and send the response.
  SetReplyHeaders(request, local_path);
  request.SetReplyStatus(HttpStatus::CODE_200_OK);
  request.SetReplyBody(input, file_size);
  return request.Reply();
}

util::StatusOr<bool> HttpStaticFileHandler::MaybeHandleRangeRequest(
    HttpRequest& request, std::string_view local_path) const {
  // Check if the client sent a range request. If an error is returned from
  // ParseRangeRequest, the client send a malformed range request, send error.
  const util::StatusOr<RangeRequest> range_request = ParseRangeRequest(request);
  if (!range_request.ok()) {
    request.SetReplyStatus(HttpStatus::CODE_400_BAD_REQUEST);
    RETURN_IF_ERROR(request.Reply());
    return true;
  }

  // Return early if the server did not send a range request.
  if (!range_request->is_range_request) return false;

  // At this point, the client sent a range request. Open the stream.
  std::ifstream input(local_path.data(), std::ios::binary);
  if (input.fail()) {
    return util::InternalError("Failed to open file");
  }

  // Generate a range response, and seek the input stream to request position.
  ASSIGN_OR_RETURN(RangeResponse range_response,
                   GenerateRangeResponse(*range_request, input));

  // Check if the range request is unsatisfiable.
  if (range_response.start == -1 || range_response.end == -1) {
    request.SetReplyStatus(HttpStatus::CODE_416_RANGE_NOT_SATISFIABLE);
    request.SetReplyHeader("Accept-Ranges", "bytes");
    request.SetReplyHeader("Content-Range",
                           GetContentRangeHeader(range_response));
    RETURN_IF_ERROR(request.Reply());
    return true;
  }

  const int64_t body_size = range_response.end - range_response.start + 1;

  SetReplyHeaders(request, local_path);
  request.SetReplyStatus(HttpStatus::CODE_206_PARTIAL_CONTENT);
  request.SetReplyHeader("Content-Range",
                         GetContentRangeHeader(range_response));
  request.SetReplyBody(input, body_size);
  return request.Reply();
}

void HttpStaticFileHandler::SetReplyHeaders(HttpRequest& request,
                                            std::string_view local_path) const {
  request.SetReplyContentType(GetContentType(local_path));
  for (const auto& [name, value] : options_.reply_headers) {
    request.SetReplyHeader(name, value);
  }
  request.SetReplyHeader("Accept-Ranges", "bytes");
}

}  // namespace net
