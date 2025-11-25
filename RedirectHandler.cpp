#include "RedirectHandler.hpp"

#include "HttpStatus.hpp"
#include "Response.hpp"
#include "constants.hpp"

int HandlerRedirect(Connection& conn, const Location& location) {
  conn.response = Response();
  conn.response.status_line.version = HTTP_VERSION;
  conn.response.status_line.status_code = location.redirect_code;
  conn.response.status_line.reason = http::reasonPhrase(location.redirect_code);
  conn.response.addHeader("Location", location.redirect_location);
  conn.response.getBody().data = "";
  conn.response.addHeader("Content-Length", "0");
  conn.write_buffer = conn.response.serialize();
   conn.write_offset = 0;
  return 0;
}
