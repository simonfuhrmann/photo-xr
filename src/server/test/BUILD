load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "tinytest",
  srcs = ["tinytest.cc"],
  hdrs = ["tinytest.h"],
)

# Tests

cc_test(
  name = "tinytest_test",
  srcs = ["tinytest_test.cc"],
  deps = [":tinytest"],
)
