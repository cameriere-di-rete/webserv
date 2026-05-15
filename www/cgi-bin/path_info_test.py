#!/usr/bin/python3

import os

print("Content-Type: text/plain")
print()

print("=== PATH_INFO Test ===")
print(f"REQUEST_URI: {os.environ.get('REQUEST_URI', 'Not set')}")
print(f"SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'Not set')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'Not set')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'Not set')}")
