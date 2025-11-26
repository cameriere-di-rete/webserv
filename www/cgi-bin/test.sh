#!/bin/bash

echo "Content-Type: text/plain"
echo ""

echo "=== Bash CGI Script Test ==="
echo "Request Method: $REQUEST_METHOD"
echo "Request URI: $REQUEST_URI"
echo "Query String: $QUERY_STRING"
echo "Server Name: $SERVER_NAME"
echo "Current Date: $(date)"
echo ""

if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ]; then
    echo "POST Data:"
    cat
fi
