#!/bin/sh
echo "Content-Type: text/plain"
echo ""
if [ -z "$HTTP_COOKIE" ]; then
  echo "NO_COOKIE"
else
  echo "$HTTP_COOKIE"
fi
