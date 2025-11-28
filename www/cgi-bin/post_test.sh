#!/bin/bash
echo "Content-Type: text/plain"
echo ""
echo "POST Test"
echo "Content-Length: $CONTENT_LENGTH"
echo "Content-Type: $CONTENT_TYPE"
echo "Body:"
head -c "$CONTENT_LENGTH"  # read up to Content-Length bytes from stdin
