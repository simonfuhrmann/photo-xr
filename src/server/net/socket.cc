#include "src/server/net/socket.h"

#include <cerrno>
#include <cstring>

#include "src/server/net/platform.h"

namespace net {

util::Status Socket::Close() {
  if (IsClosed()) return util::OkStatus();
  RETURN_IF_ERROR(Shutdown());
  RETURN_IF_ERROR(CloseSocket());
  return util::OkStatus();
}

util::Status Socket::MaybeCloseSocket(int errno_code) {
  // These are unrecoverable error codes that still require to close the socket.
  // A socket shutdown is not required, the connection is already dead.
#ifndef _WIN32
  if (errno_code == EPIPE || errno_code == ECONNRESET ||
      errno_code == ETIMEDOUT || errno_code == ENOTCONN ||
      errno_code == ECONNABORTED) {
    return CloseSocket();
  }
#else   // _WIN32
  if (error_code == WSAECONNRESET || error_code == WSAECONNABORTED ||
      error_code == WSAENETRESET || error_code == WSAETIMEDOUT ||
      error_code == WSAESHUTDOWN || error_code == WSAENOTCONN) {
    return CloseSocket();
  }
#endif  // _WIN32
  return util::OkStatus();
}

util::Status Socket::CloseSocket() {
#ifndef _WIN32
  if (::close(socket_) == kSocketError) {
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  if (::closesocket(sock_) == kSocketError) {
    return util::FailedPreconditionError("Socket error");
  }
#endif  // _WIN32
  socket_ = kInvalidSocket;
  return util::OkStatus();
}

util::Status Socket::Shutdown() {
#ifndef _WIN32
  if (::shutdown(socket_, SHUT_RDWR) == kSocketError) {
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  if (::shutdown(socket_, SD_BOTH) == kSocketError) {
    return util::FailedPreconditionError(WsaGetLastErrorString());
  }
#endif  // _WIN32
  return util::OkStatus();
}

util::Status Socket::ShutdownInput() {
#ifndef _WIN32
  if (::shutdown(socket_, SHUT_RD) == kSocketError) {
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  if (::shutdown(socket_, SD_RECEIVE) == kSocketError) {
    return util::FailedPreconditionError(WsaGetLastErrorString());
  }
#endif  // _WIN32
  return util::OkStatus();
}

util::Status Socket::ShutdownOutput() {
#ifndef _WIN32
  if (::shutdown(socket_, SHUT_WR) == kSocketError) {
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  if (::shutdown(socket_, SD_SEND) == kSocketError) {
    return util::FailedPreconditionError(WsaGetLastErrorString());
  }
#endif  // _WIN32
  return util::OkStatus();
}

bool Socket::IsInputAvailable(void) const {
  fd_set set;
  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  FD_ZERO(&set);
  FD_SET(socket_, &set);

  // select() returns 0 if timeout, 1 if input available, -1 if error.
  // TODO: Consider using poll() or epoll due to low FD_SETSIZE limits.
  return ::select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 1;
}

util::StatusOr<size_t> Socket::PartialRead(void* buffer, size_t size,
                                           size_t offset) {
#ifndef _WIN32
  const ssize_t ret = ::recv(socket_, (char*)buffer + offset, size, 0);
  if (ret == -1) {
    MaybeCloseSocket(errno).IgnoreError();
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  const int ret = ::recv(socket_, (char*)buffer + offset, size, 0);
  if (ret == SOCKET_ERROR) {
    MaybeCloseSocket(WsaGetLastError()).IgnoreError();
    return util::FailedPreconditionError(WsaGetLastErrorString());
  }
#endif  // _WIN32
  return ret;
}

util::StatusOr<size_t> Socket::Read(void* buffer, size_t size, size_t offset) {
  size_t ret = 0;
  while (ret < size) {
    ASSIGN_OR_RETURN(const size_t new_read,
                     PartialRead(buffer, size - ret, ret + offset));
    if (new_read == 0) break;  // EOF
    ret += new_read;
  }
  return ret;
}

util::StatusOr<std::string> Socket::ReadLine(size_t max_len) {
  std::string result;
  result.reserve(128);
  while (result.empty() || result.back() != '\n') {
    char buf;
    ASSIGN_OR_RETURN(const size_t new_read, PartialRead(&buf, 1, 0));
    if (new_read == 0) break;  // EOF
    result.push_back(buf);
    if (max_len > 0 && result.size() >= max_len) break;
  }
  return result;
}

util::StatusOr<size_t> Socket::PartialWrite(const void* buffer, size_t size,
                                            size_t offset) {
  const char* buf = static_cast<const char*>(buffer);
#ifndef _WIN32
  const ssize_t ret = ::send(socket_, buf + offset, size, 0);
  if (ret == -1) {
    MaybeCloseSocket(errno).IgnoreError();
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  const int ret = ::send(socket_, buf + offset, size, 0);
  if (ret == SOCKET_ERROR) {
    MaybeCloseSocket(WsaGetLastError()).IgnoreError();
    return util::FailedPreconditionError(WsaGetLastErrorString());
  }
#endif  // _WIN32
  return ret;
}

util::Status Socket::Write(const void* buffer, size_t size, size_t offset) {
  if (size == 0) return util::OkStatus();

  size_t ret = 0;
  while (ret < size) {
    ASSIGN_OR_RETURN(const size_t bytes,
                     PartialWrite(buffer, size - ret, ret + offset));
    if (bytes == 0) {
      return util::UnavailableError("Zero-byte transfer");
    }
    ret += bytes;
  }
  return util::OkStatus();
}

util::Status Socket::Write(std::string_view buffer) {
  return Write(buffer.data(), buffer.size());
}

util::Status Socket::WriteLine(std::string_view buffer) {
  RETURN_IF_ERROR(Write(buffer));
  if (buffer.empty() || buffer.back() != '\n') {
    RETURN_IF_ERROR(Write("\n"));
  }
  return util::OkStatus();
}

}  // namespace net
