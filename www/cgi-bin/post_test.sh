#!/bin/bash
echo "Content-Type: text/plain"
echo ""
echo "POST Test"
echo "Content-Length: $CONTENT_LENGTH"
echo "Content-Type: $CONTENT_TYPE"
echo "Body:"
cat  # read from stdin
