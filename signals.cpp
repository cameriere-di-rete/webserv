#include "signals.hpp"
#include "Logger.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * This module prefers using signalfd (Linux) so signals can be handled as
 * normal file-descriptor events (integrable with epoll). If signalfd isn't
 * available or creation fails, it falls back to the previous simple
 * sigaction-based flag.
 */

static volatile sig_atomic_t g_stop_requested = 0;
static int g_sfd = -1;

// Fallback handler if signalfd creation fails.
static void handle_signal(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    g_stop_requested = 1;
  }
}

// NOTE: we intentionally avoid a small helper here to keep compatibility
// with -std=c++98 and avoid unused-function warnings; we directly set
// flags where needed when necessary.

void setup_signal_handlers() {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGHUP);

  // Block signals in the process; they'll be consumed via signalfd.
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) < 0) {
    LOG_PERROR(ERROR, "signals: pthread_sigmask");
    // fallback to traditional handlers below
  } else {
    // Try to create signalfd
    g_sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (g_sfd < 0) {
      LOG_PERROR(ERROR, "signals: signalfd");
      // clear g_sfd so fallback will install handlers
      g_sfd = -1;
      // Unblock signals for fallback path
      sigprocmask(SIG_UNBLOCK, &mask, NULL);
    }
  }

  if (g_sfd < 0) {
    // fallback: install simple handlers and ignore SIGPIPE
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

    LOG(INFO)
        << "signals: fallback handlers installed (SIGINT,SIGTERM,SIGPIPE)";
  } else {
    // Also ignore SIGPIPE as before
    struct sigaction sa_pipe;
    sa_pipe.sa_handler = SIG_IGN;
    sa_pipe.sa_flags = 0;
    sigemptyset(&sa_pipe.sa_mask);
    if (sigaction(SIGPIPE, &sa_pipe, NULL) < 0) {
      LOG_PERROR(ERROR, "signals: sigaction(SIGPIPE)");
    }
    LOG(INFO) << "signals: signalfd installed and signals blocked";
  }
}

bool stop_requested() {
  return g_stop_requested != 0;
}

int signal_fd() {
  return g_sfd;
}

bool process_signals_from_fd() {
  if (g_sfd < 0)
    return false;

  struct signalfd_siginfo fdsi;
  while (1) {
    ssize_t s = read(g_sfd, &fdsi, sizeof(fdsi));
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return stop_requested();
      LOG_PERROR(ERROR, "signals: read(signalfd)");
      return stop_requested();
    }
    if (s != sizeof(fdsi)) {
      // short read - ignore
      continue;
    }

    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
      g_stop_requested = 1;
      return true;
    }
    if (fdsi.ssi_signo == SIGHUP) {
      LOG(INFO) << "signals: SIGHUP received";
      // possible reload config action could be triggered elsewhere
      continue;
    }
    LOG(INFO) << "signals: got signo=" << fdsi.ssi_signo;
  }
}
