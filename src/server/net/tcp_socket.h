#ifndef SRC_NET_TCP_SOCKET_H_
#define SRC_NET_TCP_SOCKET_H_

#include <string_view>

#include "src/server/net/platform.h"
#include "src/server/net/socket.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace net {

// TCP/IP network socket class. Only IPv4 is supported.
// This class does not throw exceptions, but uses util::Status(Or) for errors.
class TcpSocket : public Socket {
 public:
  // Creates a connected `TcpSocket` object. Internally, the `Connect()`
  // function is called with the same arguments (see `Connect()`' docs).
  static util::StatusOr<TcpSocket> Create(std::string_view host, int port);

  // Creates a connected `TcpSocket` object from an already connected socket
  // file descriptor. All members are updated with the appropriate data. This
  // constructor is used by `TcpServerSocket` for newly accepted connections.
  static util::StatusOr<TcpSocket> Create(SocketFD socket_fd);

  // Creates an unconnected `TcpSocket` object. The results of all member
  // functions but `Connect()` are undefined until the socket is connected.
  TcpSocket();
  ~TcpSocket() override = default;

  // Initiates a connection to the address specified by the `host` and `port`
  // arguments. This socket is typically on another machine, and it must be
  // already set up as a server. The `host` argument is assumed to be in the
  // standard dots-and-numbers IPv4 format, or hexadecimal IPv6 string format.
  // The overload allows passing a host in `in_addr_t` format.
  //
  // This function blocks until the server responds to the request, or the
  // previously set timeout expires.
  util::Status Connect(std::string_view host, int port);
  util::Status Connect(in_addr_t host, int port);

  // Sets a timeout in milliseconds for the `Connect()` function. `Connect()`
  // will fail if no connection has been established after the timeout expires.
  void SetConnectTimeout(int timeout_ms);

  // Returns the local/remote port.
  int GetLocalPort() const;
  int GetRemotePort() const;

  // Returns the local/remote address in the standard dots-and-numbers IPv4
  // format, or hexadecimal IPv6 string format.
  std::string GetLocalAddress() const;
  std::string GetRemoteAddress() const;

  // Returns a concatenation of the local/remote address, followed by a colon
  // (":") and the local/remote port number.
  std::string GetFullLocalAddress() const;
  std::string GetFullRemoteAddress() const;

  // Disallow copy and assign, implement move and move-assign.
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;
  TcpSocket(TcpSocket&& other) noexcept;
  TcpSocket& operator=(TcpSocket&& other) noexcept;

 private:
  sockaddr_in remote_;
  sockaddr_in local_;
  int timeout_ms_ = 0;
};

// Inline implementation.

inline TcpSocket::TcpSocket() { WsaInitOnce(); }

inline void TcpSocket::SetConnectTimeout(int timeout_ms) {
  timeout_ms_ = timeout_ms;
}

inline int TcpSocket::GetLocalPort() const { return ntohs(local_.sin_port); }

inline int TcpSocket::GetRemotePort() const { return ntohs(remote_.sin_port); }

inline TcpSocket::TcpSocket(TcpSocket&& other) noexcept {
  *this = std::move(other);
}

inline TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
  if (this == &other) return *this;

  Socket::operator=(std::move(other));
  remote_ = std::move(other.remote_);
  local_ = std::move(other.local_);
  timeout_ms_ = other.timeout_ms_;

  other.remote_ = sockaddr_in{};
  other.local_ = sockaddr_in{};
  other.timeout_ms_ = 0;
  return *this;
}

}  // namespace net

#endif  // SRC_NET_TCP_SOCKET_H_
