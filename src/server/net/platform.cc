#include "src/server/net/platform.h"

#ifdef _WIN32
#include <windows.h>

#include <mutex>
#include <stdexcept>
#include <string>
#endif  // _WIN32

namespace net {

void WsaInitOnce() {
#ifdef _WIN32
  static std::once_flag init_flag;
  std::call_once(init_flag, []() {
    WSADATA wsa_data;
    const int init_result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (init_result != 0) {
      std::cerr << "Fatal: WSAStartup failed with error code: " << result
                << std::endl;
      std::abort();
    }
  });
#endif  // _WIN32
}

#ifdef _WIN32
std::string WsaGetLastErrorString() {
  const DWORD error_code = ::WSAGetLastError();
  if (error_code == 0) return "No error";

  LPVOID message_buffer = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;

  const DWORD length = ::FormatMessageA(
      flags, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);

  std::string message;
  if (length > 0 && message_buffer != nullptr) {
    message.assign(static_cast<char*>(message_buffer), length);
    // Strip trailing newlines and carriage returns.
    while (!message.empty() &&
           (message.back() == '\n' || message.back() == '\r')) {
      message.pop_back();
    }
  } else {
    message = "Unknown WSA error code: " + std::to_string(error_code);
  }

  if (message_buffer != nullptr) {
    ::LocalFree(message_buffer);
  }

  return message;
}
#endif  // _WIN32

}  // namespace net
