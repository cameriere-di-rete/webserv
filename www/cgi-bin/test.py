#!/usr/bin/python3

import os
import sys

print("Content-Type: text/html")
print()

print("<html>")
print("<head><title>Python CGI Test</title></head>")
print("<body>")
print("<h1>Python CGI Script Working!</h1>")
print("<h2>Environment Variables:</h2>")
print("<ul>")

# Show some key CGI environment variables
cgi_vars = ['REQUEST_METHOD', 'REQUEST_URI', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH', 'SERVER_NAME', 'SERVER_PORT']
for var in cgi_vars:
    value = os.environ.get(var, 'Not set')
    print(f"<li><strong>{var}:</strong> {value}</li>")

print("</ul>")

# Show POST data if present
if os.environ.get('REQUEST_METHOD') == 'POST':
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    except (ValueError, TypeError):
        content_length = 0
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print("<h2>POST Data:</h2>")
        print(f"<pre>{post_data}</pre>")

print("</body>")
print("</html>")
