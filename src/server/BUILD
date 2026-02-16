package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "api_handler",
  hdrs = ["api_handler.h"],
  srcs = ["api_handler.cc"],
  deps = [
    "//src/server/net:http_handler",
    "//src/server/net:http_req_target",
    "//src/server/net:http_request",
    "//src/server/util:status",
    "//src/server/util:json",
  ],
)

cc_library(
  name = "web_server",
  hdrs = ["web_server.h"],
  srcs = ["web_server.cc"],
  data = ["//src/client:client_files"],
  deps = [
    ":api_handler",
    "//src/server/net:http_handler",
    "//src/server/net:http_server",
    "//src/server/util:status",
  ],
)

cc_library(
  name = "xmp_util",
  hdrs = ["xmp_util.h"],
  srcs = ["xmp_util.cc"],
  deps = [
    "//src/server/util:status_or",
  ],
)
