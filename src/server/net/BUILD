package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "http_handler",
  hdrs = ["http_handler.h"],
  srcs = ["http_handler.cc"],
  deps = [
    ":http_request",
    ":http_types",
    "//src/server/util:string_utils",
    "//src/server/util:file_utils",
  ],
)

cc_library(
  name = "http_range_request",
  hdrs = ["http_range_request.h"],
  srcs = ["http_range_request.cc"],
  deps = [
    ":http_request",
    "//src/server/util:status_or",
    "//src/server/util:string_utils",
  ],
)

cc_library(
  name = "http_req_target",
  hdrs = ["http_req_target.h"],
  srcs = ["http_req_target.cc"],
  deps = [
    "//src/server/util:string_utils",
  ],
)

cc_library(
  name = "http_request",
  hdrs = ["http_request.h"],
  srcs = ["http_request.cc"],
  deps = [
    ":http_types",
    ":socket",
    "//src/server/util:status_or",
    "//src/server/util:status",
    "//src/server/util:string_utils",
  ],
)

cc_library(
  name = "http_server",
  hdrs = ["http_server.h"],
  srcs = ["http_server.cc"],
  deps = [
    ":tcp_server_socket",
    ":http_handler",
    ":http_request",
    "//src/server/util:thread_pool",
    "//src/server/util:status",
  ],
)

cc_library(
  name = "http_types",
  hdrs = ["http_types.h"],
  srcs = ["http_types.cc"],
)

cc_library(
  name = "platform",
  hdrs = ["platform.h"],
  srcs = ["platform.cc"],
)

cc_library(
  name = "socket",
  hdrs = ["socket.h"],
  srcs = ["socket.cc"],
  deps = [
    ":platform",
    "//src/server/util:status",
    "//src/server/util:status_or",
  ],
)

cc_library(
  name = "tcp_server_socket",
  hdrs = ["tcp_server_socket.h"],
  srcs = ["tcp_server_socket.cc"],
  deps = [
    ":platform",
    ":tcp_socket",
    "//src/server/util:status",
    "//src/server/util:status_or",
  ],
)

cc_library(
  name = "tcp_socket",
  hdrs = ["tcp_socket.h"],
  srcs = ["tcp_socket.cc"],
  deps = [
    ":platform",
    ":socket",
    "//src/server/util:status",
    "//src/server/util:status_or",
  ],
)

# Tests.

cc_test(
  name = "http_range_request_test",
  srcs = ["http_range_request_test.cc"],
  size = "small",
  deps = [
    ":http_range_request",
    "//src/server/util:status_or",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "http_req_target_test",
  srcs = ["http_req_target_test.cc"],
  size = "small",
  deps = [
    ":http_req_target",
    "//src/server/util:string_utils",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "http_request_test",
  srcs = ["http_request_test.cc"],
  size = "small",
  deps = [
    ":http_request",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "http_types_test",
  srcs = ["http_types_test.cc"],
  size = "small",
  deps = [
    ":http_types",
    "//src/server/test:tinytest",
  ],
)
