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

# Run each test file
for test_file in test_*.py; do
    if [ -f "$test_file" ]; then
        echo "Running $test_file..."
        python3 "$test_file"
        echo
    fi
done

echo "All e2e tests completed!"
