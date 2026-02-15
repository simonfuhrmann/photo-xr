#include "src/server/net/http_request.h"

#include "src/server/net/platform.h"
#include "src/server/test/tinytest.h"

namespace net {

TEST(HttpReplyTest, BasicTest) {
  // Create a socket pair for testing.
  int sv[2];
  ASSERT_NE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv), -1);
  Socket client(sv[0]);
  Socket peer(sv[1]);

  // Write the request on the peer socket.
  ASSERT_OK(peer.WriteLine("GET /index.html?test HTTP/1.1"));
  ASSERT_OK(peer.WriteLine("Host: www.example.com"));
  ASSERT_OK(peer.WriteLine("User-Agent:Abc"));          // Space missing
  ASSERT_OK(peer.WriteLine("Connection :   close  "));  // Extra spaces
  ASSERT_OK(peer.WriteLine(""));

  // Read the request on the client socket.
  HttpRequest request(&client);
  ASSERT_OK(request.ReadRequest());
  EXPECT_EQ(request.GetRequestMethod(), HttpMethod::GET);
  EXPECT_EQ(request.GetRequestMethodString(), "GET");
  EXPECT_EQ(request.GetRequestVersion(), HttpVersion::HTTP_1_1);
  EXPECT_EQ(request.GetRequestVersionString(), "HTTP/1.1");
  EXPECT_EQ(request.GetRequestTarget(), "/index.html?test");
  EXPECT_EQ(request.GetRequestLine(), "GET /index.html?test HTTP/1.1");

  // Test getting request headers. The header name is case insensitive. The
  // header value is case sensitive.
  EXPECT_EQ(request.GetRequestHeader("host"), "www.example.com");
  EXPECT_EQ(request.GetRequestHeader("User-Agent"), "Abc");
  EXPECT_EQ(request.GetRequestHeader("connection"), "close");
  EXPECT_EQ(request.GetRequestHeader("CONNECTION"), "close");
  EXPECT_EQ(request.GetRequestBody(), "");

  // Prepare the reply.
  request.SetReplyStatus(HttpStatus::CODE_202_ACCEPTED);
  request.SetReplyContentType("text/plain");
  request.SetReplyHeader("Server", "custom");
  request.SetReplyBody("Custom server test", false);

  // Test the reply getters.
  EXPECT_EQ(request.GetReplyStatus(), HttpStatus::CODE_202_ACCEPTED);
  EXPECT_EQ(request.GetReplyHeader("content-type"), "text/plain");
  EXPECT_EQ(request.GetReplyBody(), "Custom server test");

  // Reply to the peer. This shouldn't close any connections.
  ASSERT_OK(request.Reply());
  EXPECT_FALSE(client.IsClosed());
  ASSERT_FALSE(peer.IsClosed());

  // Client has written, close the socket.
  EXPECT_OK(client.Close());

  // Check the reply received.
  EXPECT_EQ(peer.ReadLine().value(), "HTTP/1.1 202 Accepted\n");
  EXPECT_EQ(peer.ReadLine().value(), "connection: close\n");
  EXPECT_EQ(peer.ReadLine().value(), "content-length: 18\n");
  EXPECT_EQ(peer.ReadLine().value(), "content-type: text/plain\n");
  EXPECT_EQ(peer.ReadLine().value(), "server: custom\n");
  EXPECT_EQ(peer.ReadLine().value(), "\n");
  EXPECT_EQ(peer.ReadLine().value(), "Custom server test");  // No newline.
  EXPECT_EQ(peer.ReadLine().value(), "");                    // EOF
  EXPECT_OK(peer.Close());
}

TEST(HttpReplyTest, MalformedRequest) {
  // Create a socket pair for testing.
  int sv[2];
  ASSERT_NE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv), -1);
  Socket client(sv[0]);
  Socket peer(sv[1]);

  // Write the request on the peer socket. Use INVALID HTTP method.
  ASSERT_OK(peer.WriteLine("TEST / HTTP/1.2"));
  ASSERT_OK(peer.WriteLine("Connection: close"));
  ASSERT_OK(peer.WriteLine(""));

  // Read the request on the client socket. Reply if invalid request.
  HttpRequest request(&client);
  ASSERT_OK(request.ReadRequest());
  ASSERT_NOT_OK(request.ReplyIfInvalidRequest());
  EXPECT_FALSE(client.IsClosed());
  ASSERT_FALSE(peer.IsClosed());

  // Close the client connection.
  EXPECT_OK(client.Close());

  // Check the error reply received.
  EXPECT_EQ(peer.ReadLine().value(), "HTTP/1.1 405 Method Not Allowed\n");
  EXPECT_EQ(peer.ReadLine().value(), "connection: close\n");
  EXPECT_EQ(peer.ReadLine().value(), "content-length: 18\n");
  EXPECT_EQ(peer.ReadLine().value(), "content-type: text/plain\n");
  EXPECT_EQ(peer.ReadLine().value(), "\n");
  EXPECT_EQ(peer.ReadLine().value(), "Method Not Allowed");
  EXPECT_EQ(peer.ReadLine().value(), "");  // EOF
  EXPECT_OK(peer.Close());
}

}  // namespace net
