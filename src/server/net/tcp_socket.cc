#include "src/server/net/tcp_socket.h"

#include <cstdint>
#include <cstring>
#include <sstream>

#include "src/server/net/platform.h"

namespace net {

util::StatusOr<TcpSocket> TcpSocket::Create(std::string_view host, int port) {
  TcpSocket tcp_socket;
  RETURN_IF_ERROR(tcp_socket.Connect(host, port));
  return tcp_socket;
}

util::StatusOr<TcpSocket> TcpSocket::Create(SocketFD socket_fd) {
  TcpSocket tcp_socket;
  tcp_socket.socket_ = socket_fd;

  socklen_t remote_len = sizeof(struct sockaddr_in);
  socklen_t local_len = sizeof(struct sockaddr_in);
  if (::getpeername(socket_fd, (struct sockaddr*)&tcp_socket.remote_,
                    &remote_len) != 0) {
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }
  if (::getsockname(socket_fd, (struct sockaddr*)&tcp_socket.local_,
                    &local_len) != 0) {
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  return tcp_socket;
}

util::Status TcpSocket::Connect(std::string_view host, int port) {
  // Initialize `hints`, which specifies criteria for selecting the socket
  // address structures returned in the list pointed to by `res`.
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;  // Dual-stack IPv4 and 6.
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;

  // `getaddrinfo()` returns one or more addrinfo structures, each of which
  // contains an Internet address that can be specified in a call to `bind()`
  // or `connect()`. If successful, `res` is a linked list of addrinfo structs,
  // and the items in the linked list are linked by the `ai_next` field.
  struct addrinfo* res = nullptr;
  int retval = ::getaddrinfo(host.data(), nullptr, &hints, &res);
  if (retval != 0) {
    return util::InvalidArgumentError(::gai_strerror(retval));
  }

  // Copy the first address and free `res`.
  struct sockaddr_in tmp = *(struct sockaddr_in*)res->ai_addr;
  tmp.sin_port = htons(static_cast<uint16_t>(port));
  ::freeaddrinfo(res);

  return Connect(tmp.sin_addr.s_addr, port);
}

util::Status TcpSocket::Connect(in_addr_t host, int port) {
  // Close existing connection.
  RETURN_IF_ERROR(Close());

  socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == kInvalidSocket) {
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  remote_.sin_family = AF_INET;
  remote_.sin_port = htons(static_cast<uint16_t>(port));
  remote_.sin_addr.s_addr = host;

  // If a timeout is specified, set non-blocking flag on the socket.
  if (timeout_ms_ > 0) {
#ifndef _WIN32
    // Get the current flags.
    int flags = ::fcntl(socket_, F_GETFL, 0);
    if (flags == -1) {
      RETURN_IF_ERROR(Close());
      return util::StatusFromErrno(errno);
    }
    // Add the non-blocking flag and apply the new flags.
    flags |= O_NONBLOCK;
    if (::fcntl(socket_, F_SETFL, flags) == -1) {
      RETURN_IF_ERROR(Close());
      return util::StatusFromErrno(errno);
    }
#else   // _WIN32
    u_long flags = 1;
    if (::ioctlsocket(socket_, FIONBIO, &flags) == SOCKET_ERROR) {
      RETURN_IF_ERROR(Close());
      return util::FailedPreconditionError(WsaGetLastErrorString());
    }
#endif  // _WIN32
  }

  // Do the actual connect call.
  int connect_ret = ::connect(socket_, (struct sockaddr*)&remote_,
                              sizeof(struct sockaddr_in));

#ifndef _WIN32
  if (connect_ret == -1 && errno == EINPROGRESS)
#else   // _WIN32
  if (connect_ret == SOCKET_ERROR && ::WSAGetLastError() == WSAEWOULDBLOCK)
#endif  // _WIN32
  {
    // Non-blocking flag is set and connection is in progress.
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(socket_, &write_fds);

    struct timeval tv_timeout;
    tv_timeout.tv_sec = timeout_ms_ / 1000;
    tv_timeout.tv_usec = (timeout_ms_ % 1000) * 1000;

#ifndef _WIN32
    connect_ret =
        ::select(socket_ + 1, nullptr, &write_fds, nullptr, &tv_timeout);
    if (connect_ret == -1) {
      RETURN_IF_ERROR(Close());
      return util::StatusFromErrno(errno);
    }
    if (connect_ret == 0) {
      RETURN_IF_ERROR(Close());
      return util::DeadlineExceededError("Connection timed out");
    }
#else   // _WIN32
    connect_ret = ::select(0, nullptr, &write_fds, nullptr, &tv_timeout);
    if (connect_ret == SOCKET_ERROR) {
      RETURN_IF_ERROR(Close());
      if (::WSAGetLastError() == WSAEINPROGRESS) {
        return util::DeadlineExceededError("Connection timed out");
      }
      return util::FailedPreconditionError(WsaGetLastErrorString());
    }
#endif  // _WIN32
  } else if (connect_ret == -1) {
    // This is a connection error.
    RETURN_IF_ERROR(Close());
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  // At this point the connection is established. For further reading and
  // writing the file descriptor, remove the non-blocking flag if set before.
  if (timeout_ms_ > 0) {
#ifndef _WIN32
    /* Remove non-blocking flag from the socket. */
    int flags = ::fcntl(socket_, F_GETFL, 0);
    if (flags == -1) {
      RETURN_IF_ERROR(Close());
      return util::StatusFromErrno(errno);
    }
    flags &= ~O_NONBLOCK;
    if (::fcntl(socket_, F_SETFL, flags) == -1) {
      RETURN_IF_ERROR(Close());
      return util::StatusFromErrno(errno);
    }
#else   // _WIN32
    u_long sockopt = 0;
    if (::ioctlsocket(socket_, FIONBIO, &sockopt) == SOCKET_ERROR) {
      RETURN_IF_ERROR(Close());
      return util::FailedPreconditionError(WsaGetLastErrorString());
    }
#endif  // _WIN32
  }

  // Get information about the local socket.
  socklen_t local_len = sizeof(struct sockaddr_in);
  if (::getsockname(socket_, (struct sockaddr*)&local_, &local_len) == -1) {
    RETURN_IF_ERROR(Close());
    return util::StatusFromErrno(errno);
  }

  return util::OkStatus();
}

std::string TcpSocket::GetLocalAddress() const {
  char buffer[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &local_.sin_addr, buffer, sizeof(buffer));
  return std::string(buffer);
}

std::string TcpSocket::GetRemoteAddress() const {
  char buffer[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &remote_.sin_addr, buffer, sizeof(buffer));
  return std::string(buffer);
}

std::string TcpSocket::GetFullLocalAddress() const {
  std::stringstream ss;
  ss << GetLocalAddress() << ":" << GetLocalPort();
  return ss.str();
}

std::string TcpSocket::GetFullRemoteAddress() const {
  std::stringstream ss;
  ss << GetRemoteAddress() << ":" << GetRemotePort();
  return ss.str();
}

}  // namespace net
