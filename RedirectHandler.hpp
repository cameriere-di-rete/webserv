#pragma once

#include "Connection.hpp"
#include "Location.hpp"

// Prepare a redirect response using the provided Location.
// Returns 0 on success, non-zero on error.
int HandlerRedirect(Connection& conn, const Location& location);
