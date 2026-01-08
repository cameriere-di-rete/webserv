#include "RedirectHandler.hpp"

#include "Connection.hpp"

RedirectHandler::RedirectHandler(const Location& location)
    : location_(location) {}

RedirectHandler::~RedirectHandler() {}

HandlerResult RedirectHandler::start(Connection& conn) {
  // Prepare redirect response and serialize into connection write buffer
  conn.response.setStatus(location_.redirect_code, conn.getHttpVersion());
  conn.response.addHeader("Location", location_.redirect_location);
  conn.response.addHeader("Content-Length", "0");

  conn.write_buffer = conn.response.serialize();
  conn.write_offset = 0;
  return HR_DONE;
}

HandlerResult RedirectHandler::resume(Connection& conn) {
  (void)conn;
  return HR_DONE;  // no resume functionality for redirects
}
