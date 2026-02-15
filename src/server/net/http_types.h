#ifndef SRC_NET_HTTP_STATUS_H_
#define SRC_NET_HTTP_STATUS_H_

#include <string_view>

namespace net {

// The HTTP status code as part of the HTTP response status line.
enum class HttpStatus {
  INVALID,

  CODE_100_CONTINUE = 100,
  CODE_101_SWITCHING_PROTOS = 101,

  CODE_200_OK = 200,
  CODE_201_CREATED = 201,
  CODE_202_ACCEPTED = 202,
  CODE_203_NON_AUTH = 203,
  CODE_204_NO_CONTENT = 204,
  CODE_205_RESET_CONTENT = 205,
  CODE_206_PARTIAL_CONTENT = 206,

  CODE_300_MULTIPLE_CHOICES = 300,
  CODE_301_MOVED_PERMANENTLY = 301,
  CODE_302_FOUND = 302,
  CODE_303_SEE_OTHER = 303,
  CODE_304_NOT_MODIFIED = 304,
  CODE_305_USE_PROXY = 305,
  CODE_306_SWITCH_PROXY = 306,
  CODE_307_TEMPORARY_REDIRECT = 307,
  CODE_308_PERMANENT_REDIRECT = 308,

  CODE_400_BAD_REQUEST = 400,
  CODE_401_UNAUTHORIZED = 401,
  CODE_402_PAYMENT_REQUIRED = 402,
  CODE_403_FORBIDDEN = 403,
  CODE_404_NOT_FOUND = 404,
  CODE_405_METHOD_NOT_ALLOWED = 405,
  CODE_406_NOT_ACCEPTABLE = 406,
  CODE_407_PROXY_AUTH_REQUIRED = 407,
  CODE_408_REQUEST_TIMEOUT = 408,
  CODE_409_CONFLICT = 409,
  CODE_410_GONE = 410,
  CODE_411_LENGTH_REQUIRED = 411,
  CODE_412_PRECONDITION_FAILED = 412,
  CODE_413_REQUEST_ENTITY_TOO_LARGE = 413,
  CODE_414_REQUEST_URI_TOO_LONG = 414,
  CODE_415_UNSUPPORTED_MEDIA_TYPE = 415,
  CODE_416_RANGE_NOT_SATISFIABLE = 416,
  CODE_417_EXPECTATION_FAILED = 417,

  CODE_500_INTERNAL_SERVER_ERROR = 500,
  CODE_501_NOT_IMPLEMENTED = 501,
  CODE_502_BAD_GATEWAY = 502,
  CODE_503_SERVICE_UNAVAILABLE = 503,
  CODE_504_GATEWAY_TIMEOUT = 504,
  CODE_505_HTTP_VERSION_NOT_SUPPORTED = 505
};

// The HTTP method as part of the HTTP request.
enum class HttpMethod {
  INVALID,
  OPTIONS,
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  TRACE,
  CONNECT,
};

// The HTTP version as part of the HTTP request.
enum class HttpVersion {
  INVALID,
  HTTP_1_0,
  HTTP_1_1,
};

// Returns a string (or "Reason Phrase") for a HTTP status code.
const char* HttpCodeToString(HttpStatus code);

// Converts the HTTP method to a string, and back.
const char* HttpMethodToString(HttpMethod method);
HttpMethod HttpMethodFromString(std::string_view str);

// Converts the HTTP version to a string, and back.
const char* HttpVersionToString(HttpVersion version);
HttpVersion HttpVersionFromString(std::string_view str);

}  // namespace net

#endif  // SRC_NET_HTTP_STATUS_H_
