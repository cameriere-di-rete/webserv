#pragma once

#include <csignal>

// Install signal handlers for SIGINT, SIGTERM and SIGPIPE.
void setup_signal_handlers();

// Returns true if a termination signal was received (SIGINT or SIGTERM).
bool stop_requested();
