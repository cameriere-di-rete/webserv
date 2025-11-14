#pragma once

#include <csignal>

// Install signal handlers (prefer signalfd on Linux). This will block the
// handled signals in the process and create a signalfd if supported.
void setup_signal_handlers();

// Returns true if a termination signal was received (SIGINT or SIGTERM).
bool stop_requested();

// If signalfd is available/setup, returns its file descriptor (>=0).
// Otherwise returns -1. The caller can add this FD to epoll to receive
// signal events as readable file descriptors.
int signal_fd();

// Read and process pending signals from the signalfd (non-blocking).
// Returns true if a termination signal (SIGINT/SIGTERM) was observed.
bool process_signals_from_fd();
