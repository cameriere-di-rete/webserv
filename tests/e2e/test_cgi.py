#!/usr/bin/env python3
"""
CGI end-to-end tests for the webserv HTTP server.

These tests verify CGI script execution functionality.
"""

import os
import sys
import unittest

from webserv_test_base import WebservTestCase


class TestCGI(WebservTestCase):
    """Test CGI script execution."""

    config_file = "cgi.conf"

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
