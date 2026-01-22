#!/usr/bin/env python3
"""
End-to-end test: verify CGI infinite script is terminated by server with 504
after approximately 10 seconds.
"""

import os
import sys
import time
import unittest

from webserv_test_base import WebservTestCase


class TestCGITimeout(WebservTestCase):
    """Test that a blocking CGI is timed out by server and returns 504."""

    config_file = "complete.conf"

    def test_infinite_cgi_times_out_with_504(self):
        start = time.time()
        response, body = self.make_request("GET", "/cgi-bin/infinite.sh")
        elapsed = time.time() - start

        # Server should return 504 Gateway Timeout
        self.assertEqual(response.status, 504)

        # Ensure elapsed time is around 10 seconds (allow some tolerance)
        self.assertGreaterEqual(elapsed, 9.5)
        self.assertLess(elapsed, 15.0)


if __name__ == "__main__":
    # Validate the webserv binary exists in usual locations so this test can
    # be run directly as a script (mirrors other e2e tests).
    project_root = os.path.join(os.path.dirname(__file__), "..", "..")
    webserv_path = os.path.join(project_root, "webserv")
    if not os.path.exists(webserv_path):
        webserv_path = os.path.join(project_root, "build", "webserv")

    if not os.path.exists(webserv_path):
        print(
            "Error: webserv executable not found in ../../webserv or ../../build/webserv",
            file=sys.stderr,
        )
        print("Please build the project first with 'make' or 'cmake --build build'", file=sys.stderr)
        sys.exit(1)

    unittest.main()
