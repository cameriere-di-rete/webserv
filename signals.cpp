#include "signals.hpp"
#include "Logger.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>

static volatile sig_atomic_t g_stop_requested = 0;

static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    g_stop_requested = 1;
  }
}

void setup_signal_handlers() {
  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    LOG_PERROR(ERROR, "signals: sigaction(SIGINT)");
  }
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    LOG_PERROR(ERROR, "signals: sigaction(SIGTERM)");
  }

  struct sigaction sa_pipe;
  sa_pipe.sa_handler = SIG_IGN;
  sa_pipe.sa_flags = 0;
  sigemptyset(&sa_pipe.sa_mask);
  if (sigaction(SIGPIPE, &sa_pipe, NULL) < 0) {
    LOG_PERROR(ERROR, "signals: sigaction(SIGPIPE)");
  }

  LOG(INFO) << "signals: handlers installed (SIGINT,SIGTERM,SIGPIPE)";
}

bool stop_requested() {
  return g_stop_requested != 0;
}
