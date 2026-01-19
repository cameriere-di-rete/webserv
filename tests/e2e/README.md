# End-to-End Tests for webserv

This directory contains end-to-end tests for the webserv HTTP server using pytest.

## Requirements

- Python 3.x
- A built `webserv` executable (`/path/to/WebServer/build/webserv`)
- pytest and requests (see `requirements.txt`)

## Running the Tests

Build the webserv executable first (see the main [README](../../README.md)):

```bash
cd /path/to/WebServer
cmake -B build
cmake --build build
```

Then run the e2e tests:

```bash
# From tests/e2e directory
cd tests/e2e

# Install dependencies (if not already installed)
uv sync
# or: pip install -r requirements.txt

# Run all tests
uv run pytest -q
# or: python -m pytest -q

# Run with verbose output
uv run pytest -v
```

## Test Files

- **test_http_basic.py** - Basic HTTP functionality (GET, HEAD, 404, body limits, multiple ports)
- **test_cgi_execution.py** - CGI script execution and timeouts
- **test_edge_cases.py** - Edge cases (permissions, invalid ranges, HEAD semantics)
- **test_file_operations.py** - File delete operations
- **test_e2e.py** - Core end-to-end scenarios (root, unknown methods, custom 404s)
- **conftest.py** - Pytest fixture that starts/stops webserv server automatically

## How It Works

1. The `conftest.py` file contains a pytest session fixture (`webserv_base`) that:
   - Finds and starts the webserv binary
   - Waits for it to listen on port 8080 (5 second timeout)
   - Cleans up after all tests are done
   
2. Each test function receives the `webserv_base` parameter containing the base URL (`http://127.0.0.1:8080`)

3. Tests use the `requests` library to make HTTP requests and verify server responses

## Configuration

The tests use `conf/test.conf` which defines:
- Small body size limits (10 bytes) to test 413 responses
- Multiple servers on ports 8080 and 8081
- Custom 404 error page (`/404.html`)

## Troubleshooting

- **"webserv binary not found"**: Build the project or set `WEBSERV_BIN` environment variable
- **Tests hang or timeout**: Check that no other process is bound to ports 8080-8081
- **Address already in use**: Kill existing server processes: `pkill -f webserv`

