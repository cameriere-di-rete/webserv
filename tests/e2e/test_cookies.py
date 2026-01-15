#!/usr/bin/env python3
"""
End-to-end cookie tests using unittest and the shared WebservTestCase.
"""

import os
import sys
import unittest

from webserv_test_base import WebservTestCase


class TestCookies(WebservTestCase):
    config_file = "cgi.conf"

    def test_set_cookie_from_cgi(self):
        response, body = self.make_request("GET", "/cgi-bin/setcookie.sh")
        self.assertEqual(response.status, 200)
        # Ensure Set-Cookie header is present and contains our value
        setcookie = response.getheader("Set-Cookie")
        self.assertIsNotNone(setcookie)
        self.assertIn("sess=abc123", setcookie)

    def test_cookie_roundtrip_to_cgi(self):
        # Send a Cookie header and ensure the CGI echoes it via HTTP_COOKIE
        headers = {"Cookie": "sess=abc123"}
        response, body = self.make_request("GET", "/cgi-bin/echo_cookie.sh", headers=headers)
        self.assertEqual(response.status, 200)
        text = body.decode().strip()
        self.assertEqual(text, "sess=abc123")


if __name__ == "__main__":
    unittest.main()
