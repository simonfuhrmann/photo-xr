load("@rules_cc//cc:defs.bzl", "cc_binary")

# Binaries

cc_binary(
  name = "main",
  srcs = ["main.cc"],
  deps = [
    "//src/server:web_server",
    "//src/server/util:argument_parser",
    "//src/server/util:status",
  ],
  linkopts = ["-ljpeg"],
)
