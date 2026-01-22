#!/bin/sh
# Simple CGI script that returns a Set-Cookie header.
# If called with query `del=1` it will emit an expired cookie to delete it.
echo "Content-Type: text/plain"

# Check QUERY_STRING for deletion request
if echo "$QUERY_STRING" | grep -q "del=1"; then
	# Expire the cookie
	echo "Set-Cookie: sess=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly"
	echo ""
	echo "DELETED"
else
	echo "Set-Cookie: sess=abc123; Path=/; HttpOnly"
	echo ""
	echo "SET"
fi
