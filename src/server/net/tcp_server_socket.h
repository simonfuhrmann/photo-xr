#ifndef SRC_NET_TCP_SERVER_SOCKET_H_
#define SRC_NET_TCP_SERVER_SOCKET_H_

#include <string>
#include <string_view>

#include "src/server/net/platform.h"
#include "src/server/net/socket.h"
#include "src/server/net/tcp_socket.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace net {

// Server socket that listens and accepts connections. Only IPv4 is supported.
// This class does not throw exceptions, but uses util::Status(Or) for errors.
class TcpServerSocket {
 public:
  // Creates a bound TcpServerSocket object. Internally, `Bind()` is called.
  // Refer to `Bind()` for further information.
  static util::StatusOr<TcpServerSocket> Create(int port, int queue_length = 0);

  // Creates a bound TcpServerSocket object. Internally, `Bind()` is called.
  // Refer to `Bind()` for further information.
  static util::StatusOr<TcpServerSocket> Create(int port,
                                                std::string_view address,
                                                int queue_length = 0);

  // Creates an unbound `TcpServerSocket` object. The queue length must be set
  // before `bind' is called, or 1 is assumed.
  TcpServerSocket();
  ~TcpServerSocket();

  // Creates the server socket and bind it to a local address. This function
  // starts listening on `port` on every network address.
  util::Status Bind(int port);

  // Creates the server socket and bind it to a local address. This function
  // starts listening on `port` on the network address specified by `address`.
  //
  // The `address` is given in standard dots-and-numbers IPv4 format, or  or
  // hexadecimal IPv6 string format. For example, set `address` to "127.0.0.1"
  // to restict the server to local connections only.
  util::Status Bind(int port, std::string_view address);

  // Accepts a connection request on the server socket. The call blocks if
  // there are no connections pending. The server socket must be bound to use
  // this function. A closed TcpSocket is returned if the server socket was
  // closed while waiting for a client connection: Hence, the caller MUST check
  // TcpSocket::IsClosed() before using the connection.
  util::StatusOr<TcpSocket> Accept();

  // The `close' function closes the associated server socket file
  // descriptor and no connections can be accepted anymore.
  // The server socket can be rebound at any time.
  util::Status Close();

  // Returns true if connections are pending and ready to be accepted.
  bool HasConnectionsPending() const;

  // Returns the local port.
  int GetPort() const;

  // Returns the local address in the standard dots-and-numbers IPv4 format, or
  // hexadecimal IPv6 string format.
  std::string_view GetAddress() const;

  // Returns a concatenation of `GetAddress()`, followed by a colon (":") and
  // the local port number.
  std::string GetFullAddress() const;

  // Returns true if the server socket is closed (and unbound).
  bool IsClosed() const;

  // Sets the number of client connections that can wait in a queue before
  // being accepted. Connections that exceed the queue length are rejected.
  void SetQueueLength(int queue_length);

  // Returns the maximum number of pendig client connections.
  int GetQueueLength() const;

  // Disallow copy and assign, implement move and move-assign.
  TcpServerSocket(const TcpServerSocket&) = delete;
  TcpServerSocket& operator=(const TcpServerSocket&) = delete;
  TcpServerSocket(TcpServerSocket&& other) noexcept;
  TcpServerSocket& operator=(TcpServerSocket&& other) noexcept;

 private:
  util::Status Shutdown();
  util::Status CloseSocket();

  int socket_ = kInvalidSocket;
  int queue_length_ = 1;
  std::string address_;
  int port_ = -1;
};

// Inline implementation.

inline TcpServerSocket::TcpServerSocket() { WsaInitOnce(); }

inline int TcpServerSocket::GetPort() const { return port_; }

inline std::string_view TcpServerSocket::GetAddress() const { return address_; }

inline bool TcpServerSocket::IsClosed() const {
  return socket_ == kInvalidSocket;
}

inline void TcpServerSocket::SetQueueLength(int queue_length) {
  queue_length_ = queue_length;
}

inline int TcpServerSocket::GetQueueLength() const { return queue_length_; }

}  // namespace net

#endif  // SRC_NET_TCP_SERVER_SOCKET_H_
