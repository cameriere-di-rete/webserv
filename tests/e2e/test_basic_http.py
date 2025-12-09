#!/usr/bin/env python3
"""
Basic end-to-end tests for the webserv HTTP server.

These tests start the webserv server and make real HTTP requests to verify
that the server is working correctly.
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
        config_path = os.path.join(project_root, "conf", "default.conf")

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


class TestBasicHTTP(WebservTestCase):
    """Test basic HTTP functionality."""

    def test_get_index_html(self):
        """Test that GET / returns the index.html file."""
        response, body = self.make_request("GET", "/")
        self.assertEqual(response.status, 200)
        self.assertIn(b"html", body.lower())

    def test_get_index_html_explicit(self):
        """Test that GET /index.html returns the index.html file."""
        response, body = self.make_request("GET", "/index.html")
        self.assertEqual(response.status, 200)
        self.assertIn(b"html", body.lower())

    def test_get_nonexistent_file(self):
        """Test that GET for a nonexistent file returns 404."""
        response, body = self.make_request("GET", "/nonexistent.html")
        self.assertEqual(response.status, 404)

    def test_get_test_txt(self):
        """Test that GET /test.txt returns the test.txt file."""
        response, body = self.make_request("GET", "/test.txt")
        self.assertEqual(response.status, 200)
        content_type = response.getheader("Content-Type")
        self.assertIsNotNone(content_type)
        self.assertIn("text/plain", content_type)

    def test_head_request(self):
        """Test that HEAD / returns headers without body."""
        response, body = self.make_request("HEAD", "/")
        self.assertEqual(response.status, 200)
        # HEAD should not return a body
        self.assertEqual(len(body), 0)
        # But should return Content-Length header
        content_length = response.getheader("Content-Length")
        self.assertIsNotNone(content_length)
        self.assertGreater(int(content_length), 0)


class TestAutoindex(WebservTestCase):
    """Test directory autoindex functionality."""

    def test_autoindex_directory_listing(self):
        """Test that autoindex generates a directory listing."""
        response, body = self.make_request("GET", "/autoindex/")
        self.assertEqual(response.status, 200)
        # Check that the response contains HTML with directory listing
        self.assertIn(b"<html", body.lower())
        self.assertIn(b"index", body.lower())


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
