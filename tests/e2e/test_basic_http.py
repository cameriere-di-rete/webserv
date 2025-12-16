#!/usr/bin/env python3
"""
Basic end-to-end tests for the webserv HTTP server.

These tests start the webserv server and make real HTTP requests to verify
that the server is working correctly.
"""

import os
import sys
import unittest

from webserv_test_base import WebservTestCase


class TestBasicHTTP(WebservTestCase):
    """Test basic HTTP functionality."""

    config_file = "default.conf"

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

    config_file = "default.conf"

    def test_autoindex_directory_listing(self):
        """Test that autoindex generates a directory listing."""
        response, body = self.make_request("GET", "/autoindex/")
        self.assertEqual(response.status, 200)
        # Check that the response contains HTML with directory listing
        self.assertIn(b"<html", body.lower())
        self.assertIn(b"index", body.lower())


if __name__ == "__main__":
    # Check if webserv is built (try both locations)
    webserv_path = os.path.join(
        os.path.dirname(__file__), "..", "..", "webserv"
    )
    if not os.path.exists(webserv_path):
        webserv_path = os.path.join(
            os.path.dirname(__file__), "..", "..", "build", "webserv"
        )
    
    if not os.path.exists(webserv_path):
        print(
            f"Error: webserv executable not found in ../../webserv or ../../build/webserv",
            file=sys.stderr,
        )
        print("Please build the project first with 'make' or 'cmake --build build'", file=sys.stderr)
        sys.exit(1)

    unittest.main()
