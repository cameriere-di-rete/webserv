#include "signals.hpp"
#include <csignal>
#include <iostream>
#include <cerrno>
#include <cstring>

static volatile sig_atomic_t g_stop_requested = 0;

static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    g_stop_requested = 1;
    std::cout << "signals: termination signal received (" << sig << ")\n";
  }
  // SIGPIPE will be ignored by install; no action here.
}

void setup_signal_handlers() {
  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    std::cerr << "signals: sigaction(SIGINT) failed: " << std::strerror(errno) << "\n";
  }
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    std::cerr << "signals: sigaction(SIGTERM) failed: " << std::strerror(errno) << "\n";
  }

  // Ignore SIGPIPE (writing to closed socket should not kill the process)
  struct sigaction sa_pipe;
  sa_pipe.sa_handler = SIG_IGN;
  sa_pipe.sa_flags = 0;
  sigemptyset(&sa_pipe.sa_mask);
  if (sigaction(SIGPIPE, &sa_pipe, NULL) < 0) {
    std::cerr << "signals: sigaction(SIGPIPE) failed: " << std::strerror(errno) << "\n";
  }

  std::cout << "signals: handlers installed (SIGINT,SIGTERM,SIGPIPE)\n";
}

bool stop_requested() { return g_stop_requested != 0; }
