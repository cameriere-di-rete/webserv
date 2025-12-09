"""
Common test infrastructure for webserv end-to-end tests.
"""

import http.client
import os
import signal
import subprocess
import time
import unittest


class WebservTestCase(unittest.TestCase):
    """Base test case that manages the webserv server lifecycle."""

    server_process = None
    server_port = 8080
    server_host = "localhost"
    config_file = None  # Override in subclass

    @classmethod
    def setUpClass(cls):
        """Start the webserv server before running tests."""
        if cls.config_file is None:
            raise ValueError("config_file must be set in subclass")

        # Get the path to the webserv executable and config
        project_root = os.path.join(os.path.dirname(__file__), "..", "..")
        webserv_path = os.path.join(project_root, "webserv")
        config_path = os.path.join(project_root, "conf", cls.config_file)

        if not os.path.exists(webserv_path):
            raise RuntimeError(
                f"webserv executable not found at {webserv_path}. "
                "Please build the project first."
            )

        # Start the server from the project root directory so relative paths work
        cls.server_process = subprocess.Popen(
            [webserv_path, config_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=project_root,
            start_new_session=True,
        )

        # Give the server time to start
        time.sleep(1)

        # Check if the server is still running
        if cls.server_process.poll() is not None:
            stdout, stderr = cls.server_process.communicate()
            raise RuntimeError(
                f"Server failed to start:\nstdout: {stdout.decode()}\n"
                f"stderr: {stderr.decode()}"
            )

    @classmethod
    def tearDownClass(cls):
        """Stop the webserv server after running tests."""
        if cls.server_process:
            # Kill the entire process group
            os.killpg(os.getpgid(cls.server_process.pid), signal.SIGTERM)
            cls.server_process.wait(timeout=5)

    def make_request(self, method, path, headers=None, body=None):
        """Make an HTTP request to the test server."""
        conn = http.client.HTTPConnection(self.server_host, self.server_port)
        try:
            conn.request(method, path, body=body, headers=headers or {})
            response = conn.getresponse()
            response_body = response.read()
            return response, response_body
        finally:
            conn.close()
