#!/bin/sh
# Wrapper for php-cgi that sets required environment variables
export REDIRECT_STATUS=200
# Use the first argument as the script filename
if [ -z "$SCRIPT_FILENAME" ]; then
    export SCRIPT_FILENAME="$1"
fi
exec /usr/bin/php-cgi

