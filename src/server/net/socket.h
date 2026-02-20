#ifndef SRC_NET_SOCKET_H_
#define SRC_NET_SOCKET_H_

#include <cstddef>
#include <string>
#include <string_view>

#include "src/server/net/platform.h"
#include "src/server/util/status.h"
#include "src/server/util/status_or.h"

namespace net {

// Network socket base class that implements RAII resource management and will
// close the socket in the destructor. This class does not throw exceptions, but
// uses util::Status for errors.
class Socket {
 public:
  // Creates a connected socket from a file descriptor.
  explicit Socket(SocketFD file_descriptor);
  virtual ~Socket();

  // Closes the connection. Does nothing if already closed.
  util::Status Close();

  // Closes the reception, transmission, or both channels of the connection.
  util::Status ShutdownInput();
  util::Status ShutdownOutput();
  util::Status Shutdown();

  // Returns true if the socket is not connected to a host.
  bool IsClosed() const;

  // Returns true if input is available on the file descriptor.
  bool IsInputAvailable() const;

  // Reads up to `size` bytes, storing the results in `buffer` without a
  // terminating null character. Returns the number of bytes actually stored.
  // This might be less than `size` if there aren't this many bytes immediately
  // available. A value of zero indicates end-of-file (except if the value of
  // the `size` is also zero).
  util::StatusOr<size_t> PartialRead(void* buffer, size_t size,
                                     size_t offset = 0);

  // Reads up to `size` bytes, storing the results in `buffer` without a
  // terminating null character. Returns the number of bytes actually stored.
  // This function may block, and does not return if no bytes are immediately
  // available.
  util::StatusOr<size_t> Read(void* buffer, size_t size, size_t offset = 0);

  // Reads up to the line breaking character '\n'. None of the line breaking
  // characters such as '\r' or '\n' are removed. The string may not contain any
  // line breaking characters if there are no more characters left to read, or
  // `max_len` has been reached. Setting `max_len` to zero is treated as
  // infinite. End-of-file is indicated when the result string is empty.
  util::StatusOr<std::string> ReadLine(size_t max_len = 0);

  // Writes up to `size` bytes from `buffer` (including terminating null
  // characters). Returns the number of bytes actually written. This is less
  // than `size` if not all bytes can immediately be written.
  util::StatusOr<size_t> PartialWrite(const void* buffer, size_t size,
                                      size_t offset = 0);

  // Writes up to `size` bytes from `buffer` (including terminating null
  // characters). Returns the number of bytes actually written. This is
  // less than `size` if no more bytes can be written.
  util::Status Write(const void* buffer, size_t size, size_t offset = 0);

  // Same as `Write()`, but uses the given string for buffer and size.
  util::Status Write(std::string_view buffer);

  // Same as `Write()`, but appends '\n' if `buffer` does not end in '\n'.
  util::Status WriteLine(std::string_view buffer);

  // Returns the bare file descriptor.
  SocketFD GetSocketFileDescriptor();

  // Disallow copy and assign, implement move and move-assign.
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;

 protected:
  Socket();
  util::Status CloseSocket();
  util::Status MaybeCloseSocket(int errno_code);

  SocketFD socket_ = kInvalidSocket;
};

// Inline implementation.

inline Socket::Socket() { WsaInitOnce(); }

inline Socket::Socket(SocketFD file_descriptor) : socket_(file_descriptor) {
  WsaInitOnce();
}

inline Socket::~Socket() { Close().IgnoreError(); }

inline bool Socket::IsClosed() const { return socket_ == kInvalidSocket; }

inline SocketFD Socket::GetSocketFileDescriptor() { return socket_; }

inline Socket::Socket(Socket&& other) noexcept { *this = std::move(other); }

inline Socket& Socket::operator=(Socket&& other) noexcept {
  if (this == &other) return *this;

  Close().IgnoreError();
  socket_ = other.socket_;
  other.socket_ = kInvalidSocket;
  return *this;
}

}  // namespace net

#endif  // SRC_NET_SOCKET_H_
