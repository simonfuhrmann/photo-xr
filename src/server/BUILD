package(default_visibility = ["//visibility:public"])

# Libraries

cc_library(
  name = "web_server",
  hdrs = ["web_server.h"],
  srcs = ["web_server.cc"],
  data = ["//src/client:client_files"],
  deps = [
    "//src/server/net:http_handler",
    "//src/server/net:http_server",
    "//src/server/util:status",
  ],
)
