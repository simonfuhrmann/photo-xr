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
    "//src/server/net:http_handler",
    "//src/server/net:http_req_target",
    "//src/server/net:http_request",
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
  name = "media_handler",
  hdrs = ["media_handler.h"],
  srcs = ["media_handler.cc"],
  deps = [
    ":album_cache",
    ":stitch_vr180",
    "//src/server/util:image_io_jpeg",
    "//src/server/util:status",
    "//src/server/util:status_or",
    "//src/server/net:http_handler",
    "//src/server/net:http_request",
  ],
)

cc_library(
  name = "stitch_vr180",
  hdrs = ["stitch_vr180.h"],
  srcs = ["stitch_vr180.cc"],
  deps = [
    "//src/server/util:base64",
    "//src/server/util:image_io_jpeg",
    "//src/server/util:status_or",
    "//src/server/util:status",
    "//src/server/util:string_utils",
  ],
)


cc_library(
  name = "web_server",
  hdrs = ["web_server.h"],
  srcs = ["web_server.cc"],
  data = ["//src/client:client_files"],
  deps = [
    ":api_handler",
    ":media_handler",
    "//src/server/net:http_handler",
    "//src/server/net:http_server",
    "//src/server/util:status",
  ],
)

# Tests.

cc_test(
  name = "stitch_vr180_test",
  srcs = ["stitch_vr180_test.cc"],
  size = "small",
  deps = [
    ":stitch_vr180",
    "//src/server/test:tinytest",
  ],
  linkopts = ["-ljpeg"],
)
