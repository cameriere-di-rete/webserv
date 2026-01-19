import os
import time
import socket
import subprocess
import signal
import shutil

import pytest


ROOT_DIR = os.path.dirname(__file__)
REPO_ROOT = os.path.abspath(os.path.join(ROOT_DIR, '..', '..'))


def find_webserv_binary():
    env = os.environ.get('WEBSERV_BIN')
    if env:
        if os.path.isfile(env) and os.access(env, os.X_OK):
            return env
    candidates = [
        os.path.join(REPO_ROOT, 'build', 'webserv'),
        os.path.join(REPO_ROOT, 'webserv'),
        os.path.join(REPO_ROOT, 'bin', 'webserv'),
        os.path.join(REPO_ROOT, 'build', 'bin', 'webserv'),
    ]
    for p in candidates:
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None


@pytest.fixture(scope='session')
def webserv_base(tmp_path_factory):
    """Start webserv and yield base URL (http://127.0.0.1:8080)."""
    bin_path = find_webserv_binary()
    if not bin_path:
        pytest.exit('webserv binary not found; build it or set WEBSERV_BIN')

    # Ensure uploads folder exists under test www
    uploads = os.path.join(ROOT_DIR, 'www', 'uploads')
    os.makedirs(uploads, exist_ok=True)

    # Ensure CGI scripts are executable
    cgi_dir = os.path.join(ROOT_DIR, 'www', 'cgi-bin')
    if os.path.isdir(cgi_dir):
        for fname in os.listdir(cgi_dir):
            full = os.path.join(cgi_dir, fname)
            try:
                os.chmod(full, os.stat(full).st_mode | 0o111)
            except Exception:
                pass

    conf_path = os.path.join(ROOT_DIR, 'conf', 'test.conf')
    log_file = os.path.join(ROOT_DIR, 'webserv_pytest.log')

    out = open(log_file, 'wb')
    proc = subprocess.Popen([bin_path, conf_path], stdout=out, stderr=out, preexec_fn=os.setsid)

    # wait for listen on 8080 (and 8081 optionally)
    timeout = 5.0
    deadline = time.time() + timeout
    ready = False
    while time.time() < deadline:
        try:
            s = socket.create_connection(('127.0.0.1', 8080), timeout=0.5)
            s.close()
            ready = True
            break
        except Exception:
            time.sleep(0.2)

    if not ready:
        # dump log
        out.flush()
        out.close()
        proc.terminate()
        pytest.skip('Timed out waiting for webserv to listen; see ' + log_file)

    base = 'http://127.0.0.1:8080'
    try:
        yield base
    finally:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except Exception:
            try:
                proc.terminate()
            except Exception:
                pass
        out.flush()
        out.close()
