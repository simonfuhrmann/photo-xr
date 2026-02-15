#ifndef SRC_NET_HTTP_SERVER_H_
#define SRC_NET_HTTP_SERVER_H_

#include <list>
#include <string>

#include "src/server/net/http_handler.h"
#include "src/server/net/http_request.h"
#include "src/server/net/tcp_server_socket.h"
#include "src/server/util/status.h"
#include "src/server/util/thread_pool.h"

namespace net {

// A simple, handler based HTTP web server.
//
// When the server accepts an incoming connection, a processing thread is
// scheduled in the thread pool. If all workers are busy, the client connection
// is queued until a worker becomes available. The worker creates an HttpRequest
// object and parses the client request. Malformed HTTP requests are handled
// immediately, and will not be passed to handlers.
//
// If the request is valid, the server iterates all handlers in order to process
// the request. Once a handler replies to the client, processing stops and
// subsequent handlers are not called.
//
// If no handler replies to the client, the client connection is closed without
// replying. As such, for a well-behaving HTTP server, the last handler should
// be an error handler that always generates a 404 error page.
class HttpServer {
 public:
  struct Options {
    // The number of workers in the thread pool to handle connections.
    // Set to 1 to serialize requests (useful for development). Must be >0.
    int num_threads = 16;

    // The local server listen address and port. Currently, the server only
    // supports one address/port. See TcpServerSocket for details.
    std::string listen_address = "127.0.0.1";
    int listen_port = 8080;
  };

  // Creates the HTTP server, the creates the thread pool (with its workers).
  // This does not start the server yet. Register handlers first.
  explicit HttpServer(const Options& options);

  // Adds a request handler. Handlers are asked to process HTTP requests in
  // the same order they are registered. The first handler that sends a reply
  // to the client stops processing and no further handlers are called. If no
  // handler replies to the client, the connection is closed without reply.
  // Handler instances must outlive the HttpServer instance.
  void RegisterHandler(const HttpHandlerBase* handler);

  // Starts the HTTP server and blocks until the server socket is closed. The
  // server can be stopped by calling Stop() in another thread.
  util::Status Start();

  // Stops the HTTP server and shuts down the server socket. The server will
  // not accept new client connections, but active connections are not affected.
  util::Status Stop();

  // Returns true if the server socket is not closed.
  bool IsRunning() const;

  // Returns the options passed into the constructor.
  const Options& GetOptions() const;

  // Handles a HttpRequest by dispatching it to registered handlers. This is
  // called internally in a worker thread when new requests arrive, but can be
  // used to handle other requests. Returns true if the request was handled.
  bool HandleHttpRequest(HttpRequest& request);

  // Disallow copy and assign.
  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

 protected:
  void HandleClient(TcpSocket& client);
  util::Status HandleClientImpl(TcpSocket& client);

 private:
  const Options options_;
  std::list<const HttpHandlerBase*> handlers_;
  util::ThreadPool thread_pool_;
  net::TcpServerSocket server_socket_;
};

// Inline implementation.

inline bool HttpServer::IsRunning() const { return !server_socket_.IsClosed(); }

inline const HttpServer::Options& HttpServer::GetOptions() const {
  return options_;
}

}  // namespace net

#endif  // SRC_NET_HTTP_SERVER_H_
