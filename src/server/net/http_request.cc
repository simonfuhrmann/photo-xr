#include "src/server/net/http_request.h"

#include <cassert>
#include <sstream>
#include <string_view>
#include <utility>

#include "src/server/util/status.h"
#include "src/server/util/status_or.h"
#include "src/server/util/string_utils.h"

namespace net {
namespace {

constexpr int kLineMax = 4096;
constexpr int kHeadersMax = 1024;
constexpr int kPostSizeMax = 1024 * 1024 * 16;  // 16 MB.
constexpr std::string_view kMimeTypeFallback = "application/octet-stream";

std::string ToLowercase(std::string&& str) {
  util::ToLowercase(str);
  return str;
}

// Splits a header line at the first colon ":" and returns a lower-case and
// trimmed header name, and a trimmed header value.
util::StatusOr<std::pair<std::string, std::string>> SplitHeader(
    std::string_view line) {
  const size_t pos = line.find_first_of(':');
  if (pos == std::string_view::npos) {
    return util::InvalidArgumentError(util::StrCat("Malformed header: ", line));
  }
  std::string_view left = line.substr(0, pos);
  std::string_view right = line.substr(pos + 1);
  return std::make_pair(ToLowercase(util::TrimString(left)),
                        util::TrimString(right));
}

std::string_view GetHeaderFromMap(std::string_view name,
                                  const HttpRequest::HttpHeaders& headers) {
  std::string lc_name(name);
  util::ToLowercase(lc_name);
  const auto iter = headers.find(lc_name);
  if (iter == headers.end()) return std::string_view();
  return iter->second;
}

}  // namespace

HttpRequest::HttpRequest(Socket* socket) : socket_(socket) {
  assert(socket_ != nullptr);
}

// Basic example HTTP request:
//
//   GET /index.html HTTP/1.1
//   Host: www.example.com
//   User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
//   Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//   Accept-Language: en-US,en;q=0.5
//   Accept-Encoding: gzip, deflate, br
//   Connection: keep-alive
//
// The first line is parsed into individual members, the remaining lines are
// parsed into a map of request headers.
util::Status HttpRequest::ReadRequest() {
  // Prevent reading the request more than once.
  if (request_read_) return util::OkStatus();
  request_read_ = true;

  // Parse the request line (method, path, protocol).
  std::string method, target, version;
  {
    ASSIGN_OR_RETURN(const std::string line, socket_->ReadLine(kLineMax));
    std::stringstream ss(line);
    ss >> method >> target >> version;
  }
  request_method_ = HttpMethodFromString(method);
  request_version_ = HttpVersionFromString(version);
  request_target_ = target;

  // Read the request headers.
  while (true) {
    ASSIGN_OR_RETURN(std::string line, socket_->ReadLine(kLineMax));
    line = util::TrimString(line);
    if (line.empty()) break;  // Empty line is end of headers.
    ASSIGN_OR_RETURN(auto name_value, SplitHeader(line));
    request_headers_[name_value.first] = std::move(name_value.second);
    if (request_headers_.size() > kHeadersMax) {
      request_status_ = HttpStatus::CODE_413_REQUEST_ENTITY_TOO_LARGE;
      return util::OkStatus();
    }
  }

  // Read POST data into request_body_. ReadPostData() should rarely return an
  // error but set request_status_ with an error code.
  if (request_method_ == HttpMethod::POST) {
    RETURN_IF_ERROR(ReadPostData());
  }

  return util::OkStatus();
}

util::Status HttpRequest::ReplyIfInvalidRequest() {
  if (request_method_ == HttpMethod::INVALID) {
    SendErrorResponse(HttpStatus::CODE_405_METHOD_NOT_ALLOWED).IgnoreError();
    return util::UnimplementedError("Unsupported HTTP method");
  }
  if (request_version_ == HttpVersion::INVALID) {
    SendErrorResponse(HttpStatus::CODE_505_HTTP_VERSION_NOT_SUPPORTED)
        .IgnoreError();
    return util::UnimplementedError("Unsupported HTTP version");
  }
  if (request_status_ != HttpStatus::CODE_200_OK) {
    SendErrorResponse(request_status_).IgnoreError();
    return util::CancelledError("Invalid request");
  }
  return util::OkStatus();
}

std::string HttpRequest::GetRequestLine() const {
  return util::StrCat(GetRequestMethodString(), " ", GetRequestTarget(), " ",
                      GetRequestVersionString());
}

std::string_view HttpRequest::GetRequestHeader(std::string_view name) const {
  return GetHeaderFromMap(name, request_headers_);
}

void HttpRequest::SetReplyHeader(std::string_view name,
                                 std::string_view value) {
  std::string lc_name(name);
  util::ToLowercase(lc_name);
  reply_headers_[std::move(lc_name)] = value;
}

void HttpRequest::SetReplyContentType(std::string_view value) {
  SetReplyHeader("content-type", value);
}

void HttpRequest::SetReplyBody(std::string_view body, bool copy_data) {
  SetReplyBody(body.data(), body.size(), copy_data);
}

void HttpRequest::SetReplyBody(const char* body, size_t length,
                               bool copy_data) {
  reply_body_.clear();
  reply_body_stream_ = nullptr;
  if (copy_data) {
    reply_body_.assign(body, length);
    reply_body_ptr_ = reply_body_.data();
    reply_body_size_ = reply_body_.size();
    return;
  }
  reply_body_ptr_ = body;
  reply_body_size_ = length;
}

void HttpRequest::SetReplyBody(std::istream& stream, size_t length) {
  reply_body_.clear();
  reply_body_ptr_ = nullptr;
  reply_body_stream_ = &stream;
  reply_body_size_ = length;
}

std::string_view HttpRequest::GetReplyHeader(std::string_view name) const {
  return GetHeaderFromMap(name, reply_headers_);
}

// Basic HTTP reply:
//
//   HTTP/1.1 200 OK
//   Content-Type: text/plain
//   Content-Length: 22
//   Connection: close
//
//   Hello, this is a test.
//
// A newline separates the headers and the body.
util::Status HttpRequest::Reply() {
  if (reply_sent_) {
    return util::FailedPreconditionError("Reply was already sent");
  }
  if (socket_->IsClosed()) {
    return util::FailedPreconditionError("Client socket is closed");
  }

  // Mark this request as replied to. Various things can go wrong below, most
  // the client closing the connection early. Either way, the server should not
  // attempt to send another reply for this request.
  reply_sent_ = true;

  // Add a "Content-Length" header if not already set.
  if (reply_headers_.find("content-length") == reply_headers_.end()) {
    SetReplyHeader("content-length", util::StrCat(reply_body_size_));
  }

  // Add a "Content-Type" header if not already set.
  if (reply_headers_.find("content-type") == reply_headers_.end()) {
    SetReplyHeader("content-type", kMimeTypeFallback);
  }

  // Currently, the server does not support "Connection: keep-alive".
  // Let the client know by adding "Connection: close" to the reply.
  SetReplyHeader("connection", "close");

  // Assemble the HTTP status line.
  std::stringstream ss;
  ss << HttpVersionToString(HttpVersion::HTTP_1_1) << " ";
  ss << static_cast<int>(reply_status_) << " ";
  ss << HttpCodeToString(reply_status_) << "\n";
  RETURN_IF_ERROR(socket_->Write(ss.str()));

  // Output the HTTP headers.
  for (const auto& [name, value] : reply_headers_) {
    RETURN_IF_ERROR(socket_->Write(name));
    RETURN_IF_ERROR(socket_->Write(": ", 2));
    RETURN_IF_ERROR(socket_->Write(value));
    RETURN_IF_ERROR(socket_->Write("\n", 1));
  }

  // Separate the headers from the body, and send the body payload.
  RETURN_IF_ERROR(socket_->Write("\n", 1));
  RETURN_IF_ERROR(SendReplyBody());

  // The connection is closed when the Socket goes out of scope.
  // "Connection: keep-alive" is not supported.
  return util::OkStatus();
}

bool HttpRequest::HasReplied() const { return reply_sent_; }

util::Status HttpRequest::SendReplyBody() {
  if (reply_body_ptr_ != nullptr) {
    RETURN_IF_ERROR(socket_->Write(reply_body_ptr_, reply_body_size_));
    return util::OkStatus();
  }
  
  if (reply_body_stream_ != nullptr) {
    char buffer[4096];
    size_t bytes_left = reply_body_size_;
    while (bytes_left > 0) {
      const size_t to_read = std::min(bytes_left, sizeof(buffer));
      reply_body_stream_->read(buffer, to_read);
      const size_t bytes_read = reply_body_stream_->gcount();
      if (bytes_read == 0) break;
      RETURN_IF_ERROR(socket_->Write(buffer, bytes_read));
      bytes_left -= bytes_read;
    }
    return util::OkStatus();
  }

  return util::OkStatus();
}

util::Status HttpRequest::SendErrorResponse(HttpStatus code) {
  std::string_view code_str = HttpCodeToString(code);

  reply_headers_.clear();
  SetReplyStatus(code);
  SetReplyContentType("text/plain");
  SetReplyBody(code_str, /*copy_data=*/false);
  return Reply();
}

util::Status HttpRequest::ReadPostData() {
  std::string_view content_len = GetRequestHeader("content-length");
  if (content_len.empty()) {
    request_status_ = HttpStatus::CODE_411_LENGTH_REQUIRED;
    return util::OkStatus();
  }
  int size = 0;
  if (!util::StrToInt(content_len, &size) || size < 0) {
    request_status_ = HttpStatus::CODE_400_BAD_REQUEST;
    return util::OkStatus();
  }
  if (size > kPostSizeMax) {
    request_status_ = HttpStatus::CODE_413_REQUEST_ENTITY_TOO_LARGE;
    return util::OkStatus();
  }
  if (size == 0) return util::OkStatus();

  request_body_.resize(size, '\0');
  ASSIGN_OR_RETURN(const size_t bytes_read,
                   socket_->Read(request_body_.data(), size));
  if (bytes_read != static_cast<size_t>(size)) {
    request_status_ = HttpStatus::CODE_400_BAD_REQUEST;
    return util::OkStatus();
  }

  return util::OkStatus();
}

}  // namespace net
