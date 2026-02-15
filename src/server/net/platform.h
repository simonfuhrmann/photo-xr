#ifndef SRC_NET_PLATFORM_H_
#define SRC_NET_PLATFORM_H_

#include <string>

// Platform-specific includes. This perhaps over-includes for POSIX for most
// files, but it is easier to have it defined here once.
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#else  // _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif  // _WIN32

namespace net {

#ifdef _WIN32
using SocketFD = SOCKET;
constexpr SocketFD kInvalidSocket = INVALID_SOCKET;
constexpr int kSocketError = SOCKET_ERROR;
#else
using SocketFD = int;
constexpr SocketFD kInvalidSocket = -1;
constexpr int kSocketError = -1;
#endif  // _WIN32

// WinSock initialization (does nothing on non-Windows).
void WsaInitOnce();

#ifdef _WIN32
// Error string generation.
std::string WsaGetLastErrorString();
#endif  // _WIN32

}  // namespace net

#endif  // SRC_NET_PLATFORM_H_
