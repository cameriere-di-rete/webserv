#!/bin/sh
echo "Content-Type: text/plain"
echo
# Read CONTENT_LENGTH if provided
if [ -n "$CONTENT_LENGTH" ]; then
  dd bs=1 count=$CONTENT_LENGTH 2>/dev/null
else
  # read all from stdin
  cat -
fi
