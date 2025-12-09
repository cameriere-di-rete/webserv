#!/usr/bin/env python3
"""
CGI end-to-end tests for the webserv HTTP server.

These tests verify CGI script execution functionality.
"""

import http.client
import os
import signal
import subprocess
import sys
import time
import unittest


class WebservTestCase(unittest.TestCase):
    """Base test case that manages the webserv server lifecycle."""

    server_process = None
    server_port = 8080
    server_host = "localhost"

    @classmethod
    def setUpClass(cls):
        """Start the webserv server before running tests."""
        # Get the path to the webserv executable and config
        project_root = os.path.join(os.path.dirname(__file__), "..", "..")
        webserv_path = os.path.join(project_root, "webserv")
        config_path = os.path.join(project_root, "conf", "cgi.conf")

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
            preexec_fn=os.setsid,
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


class TestCGI(WebservTestCase):
    """Test CGI script execution."""

    def test_cgi_basic_script(self):
        """Test that a basic CGI script executes and returns output."""
        response, body = self.make_request("GET", "/cgi-bin/basic.sh")
        self.assertEqual(response.status, 200)
        # Basic CGI script should output some text
        self.assertGreater(len(body), 0)

    def test_cgi_simple_script(self):
        """Test that a simple CGI script executes correctly."""
        response, body = self.make_request("GET", "/cgi-bin/simple.sh")
        self.assertEqual(response.status, 200)
        # Simple script should output content
        self.assertGreater(len(body), 0)

    def test_cgi_post_request(self):
        """Test POST request to CGI script."""
        post_data = "test=value&foo=bar"
        headers = {
            "Content-Type": "application/x-www-form-urlencoded",
            "Content-Length": str(len(post_data)),
        }
        response, body = self.make_request(
            "POST", "/cgi-bin/post_test.sh", headers=headers, body=post_data
        )
        self.assertEqual(response.status, 200)


if __name__ == "__main__":
    # Check if webserv is built
    webserv_path = os.path.join(
        os.path.dirname(__file__), "..", "..", "webserv"
    )
    if not os.path.exists(webserv_path):
        print(
            f"Error: webserv executable not found at {webserv_path}",
            file=sys.stderr,
        )
        print("Please build the project first with 'make'", file=sys.stderr)
        sys.exit(1)

    unittest.main()
