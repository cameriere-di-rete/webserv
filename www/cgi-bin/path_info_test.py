#!/usr/bin/python3

import os

print("Content-Type: text/html")
print()

print("<html>")
print("<head><title>PATH_INFO Test</title></head>")
print("<body>")
print("<h1>PATH_INFO Test Script</h1>")
print("<table border='1'>")

# Show PATH_INFO and related variables
path_vars = ['PATH_INFO', 'REQUEST_URI', 'SCRIPT_NAME', 'QUERY_STRING']
for var in path_vars:
    value = os.environ.get(var, 'Not set')
    print(f"<tr><td><strong>{var}</strong></td><td>{value}</td></tr>")

print("</table>")
print("</body>")
print("</html>")
