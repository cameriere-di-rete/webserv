#include "EchoHandler.hpp"

#include "Connection.hpp"
#include "HttpStatus.hpp"
#include "Request.hpp"

EchoHandler::EchoHandler() {}
EchoHandler::~EchoHandler() {}

HandlerResult EchoHandler::start(Connection& conn) {
  // Prepare a simple 200 OK response that echoes the request body
  conn.response.setStatus(http::S_200_OK, conn.getHttpVersion());
  conn.response.setBodyWithContentType(conn.request.getBody().data,
                                       "text/plain; charset=utf-8");

  // Serialize entire response into write_buffer
  conn.write_buffer = conn.response.serialize();
  conn.write_offset = 0;

  return HR_DONE;
}

HandlerResult EchoHandler::resume(Connection& /*conn*/) {
  // Echo is synchronous and completes in start()
  return HR_DONE;
}
