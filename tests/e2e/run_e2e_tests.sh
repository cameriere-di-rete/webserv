#!/bin/bash
# Run all end-to-end tests for webserv

set -e

# Get the directory where this script is located
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

# Check if webserv is built (try both locations)
if [ -f "../../webserv" ]; then
    WEBSERV_PATH="../../webserv"
elif [ -f "../../build/webserv" ]; then
    WEBSERV_PATH="../../build/webserv"
else
    echo "Error: webserv executable not found in ../../webserv or ../../build/webserv"
    echo "Please build the project first with 'make' or 'cmake --build build'"
    exit 1
fi

echo "Using webserv from: $WEBSERV_PATH"

echo "Running end-to-end tests..."
echo


python3 -m unittest discover -s . -v

echo "All e2e tests completed!"
