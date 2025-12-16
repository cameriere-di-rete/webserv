# End-to-End Tests

This directory contains end-to-end tests for the webserv HTTP server.

## Requirements

- Python 3.x
- A built `webserv` executable in the project root

## Running the Tests

First, build the webserv executable (see the main [README](../../README.md) for compilation instructions).

Then run the e2e tests:

```bash
# Run all e2e tests
./run_e2e_tests.sh

# Or run individual test files
python3 test_basic_http.py
python3 test_cgi.py
```

## Test Files

- `test_basic_http.py` - Tests basic HTTP functionality (GET, HEAD, 404 errors)
- `test_cgi.py` - Tests CGI script execution

## How It Works

Each test file:
1. Starts the webserv server with an appropriate config file
2. Makes HTTP requests to the server
3. Verifies the responses
4. Cleans up by stopping the server

The tests use Python's `unittest` framework and `http.client` module to make requests.
