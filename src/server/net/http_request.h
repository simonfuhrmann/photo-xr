#ifndef SRC_NET_HTTP_REQUEST_H_
#define SRC_NET_HTTP_REQUEST_H_

#include <istream>
#include <map>
#include <string>
#include <string_view>

#include "src/server/net/http_types.h"
#include "src/server/net/socket.h"
#include "src/server/util/status.h"

namespace net {

// Wrapper around a networking client socket to parse an HTTP request and reply
// to the request. The socket is usually obtained via a TCP server socket. A
// typical use-case looks like this:
//
//   HttpRequest request(socket);
//   RETURN_IF_ERROR(request.ReadRequest());
//   RETURN_IF_ERROR(request.ReplyIfInvalidRequest());
//   // Do something with the request.
//   request.SetReplyStatus(HttpStatus::CODE_200_OK);
//   request.SetReplyContentType("text/plain");
//   request.SetReplyBody(content);
//   RETURN_IF_ERROR(request.Reply());
//
// This class does not close the socket itself. It is the responsibility of the
// user to keep the socket alive during the request/reply conversation.
class HttpRequest {
 public:
  // Data type to represent HTTP headers, e.g., "Content-Type" -> "text/plain".
  // Comparator std::less<> is explicit to support std::string_view indexing.
  using HttpHeaders = std::map<std::string, std::string, std::less<>>;

  // Sets the client socket. This class does not take ownership, the socket
  // must not be destroyed during the request/reply conversation, and the
  // socket is not automatically closed after Reply().
  HttpRequest(Socket* socket);

  //
  // Methods for reading the request. These methods are called by the HTTP
  // server itself. Handlers must not call them.
  //

  // Reads the HTTP request from the client connection.
  util::Status ReadRequest();

  // Does nothing and returns OK status if the request is valid. If the request
  // is invalid, replies an HTTP error status code to the client and returns a
  // non-OK status code.
  util::Status ReplyIfInvalidRequest();

  //
  // Information about the request.
  //

  // Returns the request method (GET, POST, ...) enum and string.
  HttpMethod GetRequestMethod() const;
  std::string_view GetRequestMethodString() const;

  // Returns the request version (HTTP/1.0, HTTP/1.1, ...) enum and string.
  HttpVersion GetRequestVersion() const;
  std::string_view GetRequestVersionString() const;

  // Returns the request target. This is NOT just a path, but the full URI, in
  // origin-form or absolute-form, potentially including URL-encoded characters
  // (e.g., %20 for space), and query parameters (after "?"), as it appears in
  // the HTTP request line. The client's URI may include a fragment (e.g.,
  // "#header1"), but the fragment is not sent to the server.
  std::string_view GetRequestTarget() const;

  // Returns the full request line (e.g., "GET /index.html HTTP/1.1"). This is
  // not the original request line sent by the client, but re-assembled from
  // the request method, path and version.
  std::string GetRequestLine() const;

  // Returns the request header map, or an individual header. If the individual
  // header is not found, an empty std::string_view is returned.
  const HttpHeaders& GetRequestHeaders() const;
  std::string_view GetRequestHeader(std::string_view name) const;

  // The body is only set if the client provided data, e.g., POST data.
  std::string_view GetRequestBody() const;

  //
  // Methods for setting the reply.
  //

  // Sets the reply HTTP status code.
  void SetReplyStatus(HttpStatus code);

  // Adds or overwrites the header `name` with `value`. It is noteworthy that
  // all reply headers are stored lower-case to avoid setting the same header
  // more than once with a different case.
  void SetReplyHeader(std::string_view name, std::string_view value);
  void SetReplyContentType(std::string_view value);

  // Sets the reply body. The request will copy the data ONLY if requested.
  // If the data is not copied, then data must stay allocated during Reply().
  void SetReplyBody(std::string_view body, bool copy_data);
  void SetReplyBody(const char* body, size_t length, bool copy_data);

  // Sets the reply body from an input stream. Up to `length` bytes are read
  // from the stream. The stream object myst stay allocated during Reply().
  void SetReplyBody(std::istream& stream, size_t length);

  //
  // Information about the reply.
  // This is just echoing what has been set earlier.
  //

  // Retuns the reply status previously set.
  HttpStatus GetReplyStatus() const;

  // Returns the reply header map, or an individual header. If the individual
  // header is not found, an empty std::string_view is returned.
  const HttpHeaders& GetReplyHeaders() const;
  std::string_view GetReplyHeader(std::string_view name) const;

  // Returns the reply body previously set. The returned string view is
  // invalidated when SetReplyBody() is called again.
  std::string_view GetReplyBody() const;

  //
  // Sending the reply.
  //

  // Sends a reply to the client. This requires that reply status code, headers
  // and body are already set. This keeps the client socket open.
  util::Status Reply();

  // Indicates if a reply was sent for this request.
  bool HasReplied() const;

  // Disallow copy and assign.
  HttpRequest(const HttpRequest&) = delete;
  HttpRequest& operator=(const HttpRequest&) = delete;

 private:
  util::Status SendReplyBody();
  util::Status SendErrorResponse(HttpStatus code);
  util::Status ReadPostData();

  Socket* socket_ = nullptr;

  // HTTP request.
  HttpMethod request_method_ = HttpMethod::INVALID;
  HttpVersion request_version_ = HttpVersion::INVALID;
  std::string request_target_;
  HttpHeaders request_headers_;
  std::string request_body_;
  bool request_read_ = false;
  HttpStatus request_status_ = HttpStatus::CODE_200_OK;

  // HTTP reply.
  HttpStatus reply_status_ = HttpStatus::INVALID;
  HttpHeaders reply_headers_;
  std::string reply_body_;
  char const* reply_body_ptr_ = nullptr;
  std::size_t reply_body_size_ = 0;
  std::istream* reply_body_stream_ = nullptr;
  bool reply_sent_ = false;
};

// Inline implementation.

inline HttpMethod HttpRequest::GetRequestMethod() const {
  return request_method_;
}

inline std::string_view HttpRequest::GetRequestMethodString() const {
  return HttpMethodToString(GetRequestMethod());
}

inline HttpVersion HttpRequest::GetRequestVersion() const {
  return request_version_;
}

inline std::string_view HttpRequest::GetRequestVersionString() const {
  return HttpVersionToString(GetRequestVersion());
}

inline const HttpRequest::HttpHeaders& HttpRequest::GetRequestHeaders() const {
  return request_headers_;
}

inline std::string_view HttpRequest::GetRequestTarget() const {
  return request_target_;
}

inline std::string_view HttpRequest::GetRequestBody() const {
  return request_body_;
}

inline void HttpRequest::SetReplyStatus(HttpStatus code) {
  reply_status_ = code;
}

inline const HttpRequest::HttpHeaders& HttpRequest::GetReplyHeaders() const {
  return reply_headers_;
}

inline std::string_view HttpRequest::GetReplyBody() const {
  return std::string_view(reply_body_ptr_, reply_body_size_);
}

inline HttpStatus HttpRequest::GetReplyStatus() const { return reply_status_; }

}  // namespace net

#endif  // SRC_NET_HTTP_REQUEST_H_
