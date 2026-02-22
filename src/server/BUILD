package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "album_cache",
  hdrs = ["album_cache.h"],
  srcs = ["album_cache.cc"],
  deps = [
    ":file_utils",
    "//src/server/util:file_utils",
    "//src/server/util:status",
    "//src/server/util:status_or",
    "//src/server/util:image_io_jpeg",
  ],
)

cc_library(
  name = "api_handler",
  hdrs = ["api_handler.h"],
  srcs = ["api_handler.cc"],
  deps = [
    ":album_cache",
    ":file_utils",
    "//src/server/util:image_io_jpeg",
    "//src/server/net:http_handler",
    "//src/server/net:http_req_target",
    "//src/server/net:http_request",
    "//src/server/util:base64",
    "//src/server/util:status",
    "//src/server/util:status_or",
    "//src/server/util:json",
  ],
)

cc_library(
  name = "file_utils",
  hdrs = ["file_utils.h"],
  srcs = ["file_utils.cc"],
  deps = [],
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
