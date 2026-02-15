#include "src/server/net/http_server.h"

#include <memory>

#include "src/server/net/http_handler.h"
#include "src/server/net/http_request.h"
#include "src/server/util/status.h"

namespace net {

HttpServer::HttpServer(const Options& options)
    : options_(options), thread_pool_(options_.num_threads) {}

void HttpServer::RegisterHandler(const HttpHandlerBase* handler) {
  handlers_.push_back(handler);
}

util::Status HttpServer::Start() {
  ASSIGN_OR_RETURN(
      server_socket_,
      TcpServerSocket::Create(options_.listen_port, options_.listen_address));

  // In a loop, accept client connections. If a newly client connection is
  // closed, then the server socket has been closed (e.g., when Stop() is
  // called). Each client is then passed to a worker in the thread pool.
  while (true) {
    ASSIGN_OR_RETURN(TcpSocket client, server_socket_.Accept());
    if (server_socket_.IsClosed() || client.IsClosed()) break;
    auto client_ptr = std::make_shared<TcpSocket>(std::move(client));
    thread_pool_.Dispatch([this, client_ptr]() { HandleClient(*client_ptr); });
  }
  return util::OkStatus();
}

util::Status HttpServer::Stop() { return server_socket_.Close(); }

bool HttpServer::HandleHttpRequest(HttpRequest& request) {
  // Call handlers in order, stop processing if a handler returns OK.
  // Also stop processing if a handler returns non-OK but already replied.
  for (const HttpHandlerBase* handler : handlers_) {
    if (handler->Handle(request).ok()) return true;
    if (request.HasReplied()) return true;
  }
  return false;
}

void HttpServer::HandleClient(TcpSocket& client) {
  HandleClientImpl(client).IgnoreError();
}

util::Status HttpServer::HandleClientImpl(TcpSocket& client) {
  // Create HttpRequest for this client and read the request. If the request
  // cannot be read, return and close the connection. If the request validation
  // fails, an error response is sent, and the connection is closed.
  HttpRequest request(&client);
  RETURN_IF_ERROR(request.ReadRequest());
  RETURN_IF_ERROR(request.ReplyIfInvalidRequest());

  // Pass the request to handlers. The client connection is closed when the
  // TcpSocket shared_ptr goes out of scope.
  HandleHttpRequest(request);
  return util::OkStatus();
}

}  // namespace net
