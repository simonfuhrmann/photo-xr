#include "src/server/net/http_types.h"

namespace net {

#define HC_CASE(enum, text) \
  case HttpStatus::enum:    \
    return text

const char* HttpCodeToString(HttpStatus code) {
  switch (code) {
    HC_CASE(CODE_100_CONTINUE, "Continue");
    HC_CASE(CODE_101_SWITCHING_PROTOS, "Switching Protocols");
    HC_CASE(CODE_200_OK, "OK");
    HC_CASE(CODE_201_CREATED, "Created");
    HC_CASE(CODE_202_ACCEPTED, "Accepted");
    HC_CASE(CODE_203_NON_AUTH, "Non-Authoritative Information");
    HC_CASE(CODE_204_NO_CONTENT, "No Content");
    HC_CASE(CODE_205_RESET_CONTENT, "Reset Content");
    HC_CASE(CODE_206_PARTIAL_CONTENT, "Partial Content");
    HC_CASE(CODE_300_MULTIPLE_CHOICES, "Multiple Choices");
    HC_CASE(CODE_301_MOVED_PERMANENTLY, "Moved Permanently");
    HC_CASE(CODE_302_FOUND, "Found");
    HC_CASE(CODE_303_SEE_OTHER, "See Other");
    HC_CASE(CODE_304_NOT_MODIFIED, "Not Modified");
    HC_CASE(CODE_305_USE_PROXY, "Use Proxy");
    HC_CASE(CODE_306_SWITCH_PROXY, "Switch Proxy");
    HC_CASE(CODE_307_TEMPORARY_REDIRECT, "Temporary Redirect");
    HC_CASE(CODE_308_PERMANENT_REDIRECT, "Permanent Redirect");
    HC_CASE(CODE_400_BAD_REQUEST, "Bad Request");
    HC_CASE(CODE_401_UNAUTHORIZED, "Unauthorized");
    HC_CASE(CODE_402_PAYMENT_REQUIRED, "Payment Required");
    HC_CASE(CODE_403_FORBIDDEN, "Forbidden");
    HC_CASE(CODE_404_NOT_FOUND, "Not Found");
    HC_CASE(CODE_405_METHOD_NOT_ALLOWED, "Method Not Allowed");
    HC_CASE(CODE_406_NOT_ACCEPTABLE, "Not Acceptable");
    HC_CASE(CODE_407_PROXY_AUTH_REQUIRED, "Proxy Authentication Required");
    HC_CASE(CODE_408_REQUEST_TIMEOUT, "Request Timeout");
    HC_CASE(CODE_409_CONFLICT, "Conflict");
    HC_CASE(CODE_410_GONE, "Gone");
    HC_CASE(CODE_411_LENGTH_REQUIRED, "Length Required");
    HC_CASE(CODE_412_PRECONDITION_FAILED, "Precondition Failed");
    HC_CASE(CODE_413_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large");
    HC_CASE(CODE_414_REQUEST_URI_TOO_LONG, "Request-URI Too Large");
    HC_CASE(CODE_415_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type");
    HC_CASE(CODE_416_RANGE_NOT_SATISFIABLE, "Range Not Satisfiable");
    HC_CASE(CODE_417_EXPECTATION_FAILED, "Expectation Failed");
    HC_CASE(CODE_500_INTERNAL_SERVER_ERROR, "Internal Server Error");
    HC_CASE(CODE_501_NOT_IMPLEMENTED, "Not Implemented");
    HC_CASE(CODE_502_BAD_GATEWAY, "Bad Gateway");
    HC_CASE(CODE_503_SERVICE_UNAVAILABLE, "Service Unavailable");
    HC_CASE(CODE_504_GATEWAY_TIMEOUT, "Gateway Timeout");
    HC_CASE(CODE_505_HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported");
    default:
      break;
  }
  return "Unknown Status";
}

#define METHOD_CASE(enum) \
  case HttpMethod::enum:  \
    return #enum

const char* HttpMethodToString(HttpMethod method) {
  switch (method) {
    METHOD_CASE(OPTIONS);
    METHOD_CASE(GET);
    METHOD_CASE(HEAD);
    METHOD_CASE(POST);
    METHOD_CASE(PUT);
    METHOD_CASE(DELETE);
    METHOD_CASE(TRACE);
    METHOD_CASE(CONNECT);
    default:
      break;
  }
  return "INVALID";
}

#define METHOD_IF(enum) \
  if (str == #enum) return HttpMethod::enum

HttpMethod HttpMethodFromString(std::string_view str) {
  METHOD_IF(OPTIONS);
  METHOD_IF(GET);
  METHOD_IF(HEAD);
  METHOD_IF(POST);
  METHOD_IF(PUT);
  METHOD_IF(DELETE);
  METHOD_IF(TRACE);
  METHOD_IF(CONNECT);
  return HttpMethod::INVALID;
}

HttpVersion HttpVersionFromString(std::string_view str) {
  if (str == "HTTP/1.0") return HttpVersion::HTTP_1_0;
  if (str == "HTTP/1.1") return HttpVersion::HTTP_1_1;
  return HttpVersion::INVALID;
}

const char* HttpVersionToString(HttpVersion version) {
  if (version == HttpVersion::HTTP_1_0) return "HTTP/1.0";
  if (version == HttpVersion::HTTP_1_1) return "HTTP/1.1";
  return "INVALID";
}

}  // namespace net
