load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "argument_parser",
  hdrs = ["argument_parser.h"],
  srcs = ["argument_parser.cc"],
  deps = [
    ":status",
    ":status_or",
  ],
)

cc_library(
  name = "base64",
  hdrs = ["base64.h"],
  srcs = ["base64.cc"],
  deps = [":status_or"],
)

cc_library(
  name = "file_utils",
  hdrs = ["file_utils.h"],
  srcs = ["file_utils.cc"],
  deps = [
    ":status_or",
    ":string_utils",
  ]
)

cc_library(
  name = "image_io_jpeg",
  hdrs = ["image_io_jpeg.h"],
  srcs = ["image_io_jpeg.cc"],
  deps = [
    ":status",
    ":status_or",
  ],
)

cc_library(
  name = "json",
  hdrs = ["json.h"],
  srcs = ["json.cc"],
  deps = [":string_utils"]
)

cc_library(
  name = "status_or",
  hdrs = ["status_or.h"],
  srcs = ["status_or.cc"],
  deps = [":status"],
)

cc_library(
  name = "status",
  hdrs = ["status.h"],
  srcs = ["status.cc"],
  deps = [":string_utils"],
)

cc_library(
  name = "string_utils",
  hdrs = ["string_utils.h"],
  srcs = ["string_utils.cc"],
)

cc_library(
  name = "thread_pool",
  hdrs = ["thread_pool.h"],
)

# Tests

cc_test(
  name = "argument_parser_test",
  srcs = ["argument_parser_test.cc"],
  size = "small",
  deps = [
    ":argument_parser",
    ":status_or",
    ":status",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "base64_test",
  srcs = ["base64_test.cc"],
  size = "small",
  deps = [
    ":base64",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "file_utils_test",
  srcs = ["file_utils_test.cc"],
  size = "small",
  deps = [
    ":file_utils",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "image_io_jpeg_test",
  srcs = ["image_io_jpeg_test.cc"],
  size = "small",
  deps = [
    ":image_io_jpeg",
    ":status",
    ":status_or",
    "//src/server/test:tinytest",
  ],
  linkopts = ["-ljpeg"],
)

cc_test(
  name = "json_test",
  srcs = ["json_test.cc"],
  size = "small",
  deps = [
    ":json",
    ":status",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "status_or_test",
  srcs = ["status_or_test.cc"],
  size = "small",
  deps = [
    ":status_or",
    ":status",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "status_test",
  srcs = ["status_test.cc"],
  size = "small",
  deps = [
    ":status",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "string_utils_test",
  srcs = ["string_utils_test.cc"],
  size = "small",
  deps = [
    ":string_utils",
    "//src/server/test:tinytest",
  ],
)

cc_test(
  name = "thread_pool_test",
  srcs = ["thread_pool_test.cc"],
  size = "small",
  deps = [
    ":thread_pool",
    "//src/server/test:tinytest",
  ],
)
