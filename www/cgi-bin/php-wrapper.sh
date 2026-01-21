#!/bin/sh
export REDIRECT_STATUS=200
export SCRIPT_FILENAME="$1"
exec /usr/bin/php-cgi "$1"
