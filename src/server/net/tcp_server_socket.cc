#include "src/server/net/tcp_server_socket.h"

#include <sstream>
#include <string_view>

#include "src/server/net/platform.h"
#include "src/server/net/tcp_socket.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace net {

TcpServerSocket::~TcpServerSocket() { Close().IgnoreError(); }

util::StatusOr<TcpServerSocket> TcpServerSocket::Create(int port,
                                                        int queue_length) {
  TcpServerSocket socket;
  socket.SetQueueLength(queue_length);
  RETURN_IF_ERROR(socket.Bind(port));
  return socket;
}

util::StatusOr<TcpServerSocket> TcpServerSocket::Create(
    int port, std::string_view address, int queue_length) {
  TcpServerSocket socket;
  socket.SetQueueLength(queue_length);
  RETURN_IF_ERROR(socket.Bind(port, address));
  return socket;
}

util::Status TcpServerSocket::Bind(int port) { return Bind(port, ""); }

util::Status TcpServerSocket::Bind(int port, std::string_view address) {
  // Close socket first if not in closed state already.
  RETURN_IF_ERROR(Close());

  socket_ = ::socket(PF_INET, SOCK_STREAM, 0);
  if (socket_ == kInvalidSocket) {
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  port_ = port;
  address_ = address;

  // Give the socket a name.
  struct sockaddr_in socket_name;
  socket_name.sin_family = AF_INET;
  socket_name.sin_port = htons(static_cast<uint16_t>(port));
  if (address_.length() == 0) {
    socket_name.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    socket_name.sin_addr.s_addr = ::inet_addr(address.data());
  }

  // Allow reuse of server address even in TCP's TIME_WAIT.
  int sockopt = 1;
  if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &sockopt,
                   sizeof(sockopt)) == kSocketError) {
    RETURN_IF_ERROR(CloseSocket());
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  // Bind the socket to the local Internet address.
  if (::bind(socket_, (struct sockaddr*)&socket_name,
             sizeof(struct sockaddr_in)) == kSocketError) {
    RETURN_IF_ERROR(CloseSocket());
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }

  // Set the socket in passive listening mode to accept connections.
  if (::listen(socket_, queue_length_) == kSocketError) {
    RETURN_IF_ERROR(CloseSocket());
#ifndef _WIN32
    return util::StatusFromErrno(errno);
#else   // _WIN32
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }
  return util::OkStatus();
}

util::StatusOr<TcpSocket> TcpServerSocket::Accept() {
  if (IsClosed()) {
    return util::FailedPreconditionError("Socket is not bound");
  }

  struct sockaddr_in remote;
  socklen_t remote_len = sizeof(struct sockaddr_in);
  const int sock_fd = ::accept(socket_, (struct sockaddr*)&remote, &remote_len);

  // accept() unblocks and returns an error if the server socket is closed. In
  // this case, detect the error and return an unconnected socket instead of
  // an error. This happens, e.g., during server shutdown.
  if (sock_fd == kSocketError) {
#ifndef _WIN32
    if (errno == EINVAL) return TcpSocket();
    return util::StatusFromErrno(errno);
#else   // _WIN32
    const int error = WSAGetLastError();
    if (error == WSAEINVAL || error == WSAENOTSOCK) return TcpSocket();
    return util::FailedPreconditionError(WsaGetLastErrorString());
#endif  // _WIN32
  }
  return TcpSocket::Create(sock_fd);
}

util::Status TcpServerSocket::Close() {
  if (IsClosed()) return util::OkStatus();

  // Shut down the socket to unlock ::accept(), then close the socket.
  RETURN_IF_ERROR(Shutdown());
  RETURN_IF_ERROR(CloseSocket());

  socket_ = kInvalidSocket;
  return util::OkStatus();
}

bool TcpServerSocket::HasConnectionsPending() const {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(socket_, &set);

  struct timeval tv_timeout;
  tv_timeout.tv_sec = 0;
  tv_timeout.tv_usec = 0;

  /* select returns 0 if timeout, 1 if input available, -1 if error. */
  return ::select(FD_SETSIZE, &set, nullptr, nullptr, &tv_timeout) == 1;
}

std::string TcpServerSocket::GetFullAddress() const {
  std::stringstream ss;
  ss << GetAddress() << ":" << GetPort();
  return ss.str();
}

TcpServerSocket::TcpServerSocket(TcpServerSocket&& other) noexcept {
  *this = std::move(other);
}

TcpServerSocket& TcpServerSocket::operator=(TcpServerSocket&& other) noexcept {
  if (this == &other) return *this;

  socket_ = other.socket_;
  queue_length_ = other.queue_length_;
  address_ = std::move(other.address_);
  port_ = other.port_;

  other.socket_ = -1;
  other.queue_length_ = 1;
  other.address_.clear();
  other.port_ = -1;
  return *this;
}

util::Status TcpServerSocket::Shutdown() {
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

util::Status TcpServerSocket::CloseSocket() {
#ifndef _WIN32
  if (::close(socket_) == -1) {
    return util::StatusFromErrno(errno);
  }
#else   // _WIN32
  if (::closesocket(sock_) == SOCKET_ERROR) {
    return util::FailedPreconditionError("Socket error");
  }
#endif  // _WIN32
  return util::OkStatus();
}

}  // namespace net
