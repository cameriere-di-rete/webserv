# AGENTS.md

This document provides guidance for AI coding agents working on the **webserv** project.

## Project Overview

**webserv** is an HTTP/1.0 web server implementation written in C++98, following [RFC 1945](https://datatracker.ietf.org/doc/html/rfc1945). The project uses an event-driven, non-blocking I/O model with `epoll()` for handling concurrent connections efficiently.

### Key Design Principles

- **Non-blocking I/O**: All socket operations must be non-blocking
- **Event-driven architecture**: Uses `epoll` for multiplexing I/O across connections
- **Handler pattern**: Request processing is delegated to specialized handlers (`FileHandler`, `CgiHandler`, `AutoindexHandler`, etc.)
- **nginx-like configuration**: Familiar syntax for server configuration

## Language and Standards

| Context | Standard | Notes |
|---------|----------|-------|
| Production code (`src/`) | **C++98** | Strict requirement - no C++11+ features |
| Test code (`*_test.cpp`) | **C++11** | Required for GoogleTest compatibility |

**Compiler Flags**: `-Wall -Wextra -Werror` (all warnings treated as errors)

### C++98 Restrictions

When writing production code, avoid:
- `auto` keyword
- Range-based for loops
- `nullptr` (use `NULL`)
- Lambda expressions
- `std::unique_ptr`, `std::shared_ptr`
- `override`, `final` keywords
- Initializer lists `{}`
- `constexpr`

## Build System

### Primary: CMake (Recommended)

```bash
# Configure and build (one-liner)
mkdir -p build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -- -j$(nproc)

# Run all tests (unit + e2e)
ctest --test-dir build --output-on-failure -j$(nproc)

# Run only unit tests
./build/tests/runTests

# Run only e2e tests
cd tests/e2e && ./run_e2e_tests.sh
```

### Debug Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -- -j$(nproc)
```

### Fallback: Makefile

A generated Makefile is provided for environments without CMake:

```bash
make        # Build
make clean  # Clean build artifacts
make re     # Rebuild from scratch
```

**Note**: The Makefile is auto-generated from CMake. If CMake is available, it will auto-regenerate when `CMakeLists.txt` changes.

## Project Architecture

### Directory Structure

```
src/
├── core/           # Server core: main loop, connection management
│   ├── main.cpp           # Entry point, argument parsing
│   ├── ServerManager.*    # Main event loop using epoll, manages all servers
│   ├── Server.*           # Listening socket and server configuration
│   └── Connection.*       # Individual client connection lifecycle
├── http/           # HTTP protocol implementation
│   ├── Request.*          # HTTP request parsing
│   ├── Response.*         # HTTP response construction
│   ├── RequestLine.*      # Request line parsing (method, URI, version)
│   ├── StatusLine.*       # Status line construction
│   ├── Header.*           # HTTP header parsing/construction
│   ├── Body.*             # Request/response body handling
│   ├── Uri.*              # URI parsing and manipulation
│   ├── HttpMethod.*       # HTTP method enumeration
│   └── Message.*          # Base class for Request/Response
├── config/         # Configuration parsing (nginx-like syntax)
│   ├── Config.*           # Main configuration parser
│   ├── Location.*         # Location block handling
│   ├── BlockNode.*        # AST node for config blocks
│   └── DirectiveNode.*    # AST node for config directives
├── handlers/       # Request handlers (Strategy pattern)
│   ├── IHandler.*         # Abstract handler interface
│   ├── FileHandler.*      # Serves static files
│   ├── CgiHandler.*       # Executes CGI scripts
│   ├── AutoindexHandler.* # Generates directory listings
│   ├── RedirectHandler.*  # HTTP redirects
│   └── EchoHandler.*      # Debug/test handler
└── utils/          # Utilities
    ├── Logger.*           # Logging (DEBUG, INFO, ERROR levels)
    ├── file_utils.*       # File operations helpers
    ├── utils.*            # String utilities, misc helpers
    └── constants.hpp      # Global constants

conf/               # Configuration files
├── default.conf           # Default server configuration
├── cgi.conf               # CGI-enabled configuration
└── mime.types             # MIME type mappings

tests/              # Test suites
├── CMakeLists.txt         # Test build configuration
├── test_main.cpp          # GoogleTest main
└── e2e/                   # End-to-end tests (Python)
    ├── run_e2e_tests.sh   # E2E test runner
    ├── test_basic_http.py # Basic HTTP tests
    └── test_cgi.py        # CGI tests

www/                # Default document root
└── cgi-bin/               # CGI scripts directory

docs/               # Documentation
├── CONFIGURATION.md       # Detailed configuration guide
└── config-schema.json     # JSON schema for config structure
```

### Key Components

| Component | File | Description |
|-----------|------|-------------|
| `ServerManager` | `src/core/ServerManager.*` | Main event loop using `epoll`, manages multiple server instances and connections |
| `Server` | `src/core/Server.*` | Represents a listening socket with its configuration |
| `Connection` | `src/core/Connection.*` | Manages individual client connection state and request/response lifecycle |
| `Config` | `src/config/Config.*` | Parses nginx-like configuration files into server configurations |
| `Location` | `src/config/Location.*` | Represents a location block with URI matching |
| `Request` | `src/http/Request.*` | HTTP request parsing (inherits from `Message`) |
| `Response` | `src/http/Response.*` | HTTP response construction (inherits from `Message`) |
| `IHandler` | `src/handlers/IHandler.*` | Abstract handler interface for request processing |
| `FileHandler` | `src/handlers/FileHandler.*` | Serves static files with proper MIME types |
| `CgiHandler` | `src/handlers/CgiHandler.*` | Executes CGI scripts with environment setup |
| `AutoindexHandler` | `src/handlers/AutoindexHandler.*` | Generates HTML directory listings |
| `RedirectHandler` | `src/handlers/RedirectHandler.*` | HTTP 3xx redirects |

### Handler Interface

All handlers implement the `IHandler` interface:

```cpp
class IHandler {
 public:
  virtual ~IHandler() {}

  // Start processing - called once when handler is first invoked
  // Returns: HR_DONE (complete), HR_WOULD_BLOCK (needs more I/O), HR_ERROR (failure)
  virtual HandlerResult start(Connection& conn) = 0;

  // Continue processing after I/O is ready (for streaming, CGI, etc.)
  virtual HandlerResult resume(Connection& conn) = 0;

  // Returns FD to monitor for I/O readiness (e.g., CGI pipe), or -1 if none
  virtual int getMonitorFd() const;
};
```

## Code Style

### Formatting

- **Tool**: `clang-format` with Google style base
- **Config**: `.clang-format` (minimal customization over Google style)
- **Auto-format**: Comment `/format` on a PR to trigger automatic formatting

```yaml
# .clang-format
BasedOnStyle: Google
Language: Cpp
AllowShortFunctionsOnASingleLine: Empty
```

### Linting

- **Tool**: `clang-tidy`
- **Config**: `.clang-tidy`
- **Key check**: `readability-braces-around-statements` (braces required for ALL control statements)

```cpp
// BAD - will fail lint
if (condition)
  doSomething();

// GOOD
if (condition) {
  doSomething();
}
```

### Naming Conventions

| Element | Style | Example |
|---------|-------|---------|
| Classes | `PascalCase` | `ServerManager`, `CgiHandler` |
| Methods/Functions | `camelCase` | `parseFile()`, `matchLocation()` |
| Member variables | `snake_case_` (trailing underscore for private) | `read_buffer_`, `efd_` |
| Local variables | `snake_case` | `conn_fd`, `request_line` |
| Constants | `kPascalCase` | `kGlobalContext`, `kDefaultPort` |
| Namespaces | `lowercase` | `http` |
| Enums | `UPPER_CASE` | `HR_DONE`, `GET`, `POST` |

### Header Guards

Use `#pragma once` (not traditional include guards).

## Testing

### Framework

GoogleTest (GTest) v1.14.0, fetched via CMake FetchContent if not installed locally.

### Test Organization

Test files follow the pattern `*_test.cpp` and are placed **alongside their source files**:

```
src/config/Config.cpp        → src/config/Config_test.cpp
src/core/Connection.cpp      → src/core/Connection_test.cpp
src/http/Uri.cpp             → src/http/Uri_test.cpp
src/handlers/FileHandler.cpp → src/handlers/FileHandler_test.cpp
```

### Running Tests

```bash
# All tests
ctest --test-dir build --output-on-failure -j$(nproc)

# Unit tests only
./build/tests/runTests

# Specific test suite
./build/tests/runTests --gtest_filter="ConfigTest.*"

# E2E tests (requires built webserv)
cd tests/e2e && ./run_e2e_tests.sh
```

### Writing Tests

```cpp
#include <gtest/gtest.h>
#include "YourClass.hpp"

// Test fixture for shared setup
class YourClassTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Common setup
  }
};

TEST_F(YourClassTest, MethodDoesExpectedBehavior) {
  // Arrange
  YourClass obj;

  // Act
  int result = obj.method();

  // Assert
  EXPECT_EQ(42, result);
}

// Standalone test
TEST(YourClassTest, StaticMethodWorks) {
  EXPECT_TRUE(YourClass::staticMethod());
}
```

## Configuration

The server uses nginx-like configuration syntax. See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for full documentation.

### Example Configuration

```nginx
# Global directives
max_request_body 4096;
error_page 404 /404.html;

server {
  listen 8080;
  root ./www;
  autoindex on;
  index index.html;

  location /api {
    allow_methods GET POST;
  }

  location /cgi-bin {
    cgi on;
    allow_methods GET POST;
  }

  location /old {
    redirect 301 /new;
  }
}
```

### Supported Directives

| Directive | Context | Description |
|-----------|---------|-------------|
| `listen` | server | Port (and optionally host) to listen on. Required. |
| `root` | server, location | Document root directory. Required at server level. |
| `index` | server, location | Default index file(s) |
| `autoindex` | server, location | Enable directory listing (`on`/`off`) |
| `allow_methods` | server, location | Allowed HTTP methods (`GET`, `POST`, `PUT`, `DELETE`, `HEAD`) |
| `error_page` | global, server, location | Custom error pages (4xx, 5xx codes) |
| `max_request_body` | global, server, location | Maximum request body size in bytes |
| `cgi` | location | Enable CGI execution (`on`/`off`) |
| `redirect` | location | HTTP redirect (301, 302, 303, 307, 308) |

## Logging

Use the `LOG()` macro with levels: `DEBUG`, `INFO`, `ERROR`

```cpp
#include "Logger.hpp"

LOG(INFO) << "Server started on port " << port;
LOG(ERROR) << "Failed to open file: " << filename;
LOG(DEBUG) << "Processing request: " << request.uri;

// For errno-related errors
LOG_PERROR(ERROR, "Failed to open socket");
```

Set log level at runtime:
```bash
./webserv -l:0 config.conf  # DEBUG level
./webserv -l:1 config.conf  # INFO level (default)
./webserv -l:2 config.conf  # ERROR level
```

## CI/CD Checks

Pull requests must pass all checks:

| Check | File | What it verifies |
|-------|------|------------------|
| **Build** | `.github/workflows/build-and-test.yml` | CMake build succeeds |
| **Lint** | `.github/workflows/check-lint.yml` | `clang-tidy` passes |
| **Tests** | `.github/workflows/check-tests.yml` | All unit + e2e tests pass |
| **Makefile** | `.github/workflows/check-makefile.yml` | Generated Makefile is up-to-date |

## Common Tasks

### Adding a New Source File

1. Create `.cpp` and `.hpp` files in the appropriate `src/` subdirectory
2. Add the `.cpp` file to the subdirectory's `CMakeLists.txt`:
   ```cmake
   set_property(GLOBAL APPEND PROPERTY ALL_SOURCES
       ${CMAKE_CURRENT_SOURCE_DIR}/NewFile.cpp
   )
   ```
3. Rebuild to regenerate the Makefile

### Adding a New Handler

1. Create `NewHandler.cpp` and `NewHandler.hpp` in `src/handlers/`
2. Inherit from `IHandler`:
   ```cpp
   class NewHandler : public IHandler {
    public:
     HandlerResult start(Connection& conn);
     HandlerResult resume(Connection& conn);
     int getMonitorFd() const;  // Return -1 if no FD to monitor
   };
   ```
3. Register the handler in the connection routing logic (in `Connection.cpp`)
4. Add to `src/handlers/CMakeLists.txt`

### Adding Tests

1. Create `*_test.cpp` file alongside the source file
2. Add the test file to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(runTests
     ...
     ${CMAKE_SOURCE_DIR}/src/path/YourFile_test.cpp
   )
   ```
3. Run tests with `ctest`

### Adding a New Configuration Directive

1. Update `src/config/Config.cpp` to parse the new directive
2. Update `src/config/Location.hpp`/`Server.hpp` to store the value
3. Update `docs/CONFIGURATION.md` with documentation
4. Update `docs/config-schema.json` with schema definition
5. Add tests in `src/config/Config_test.cpp`

## Important Constraints

### Strict Requirements

- ❌ **No C++11 features** in production code (`src/`)
- ❌ **No external dependencies** beyond standard library (except GoogleTest for tests)
- ❌ **No blocking I/O** - all socket operations must be non-blocking
- ❌ **No memory leaks** - use RAII patterns, properly clean up resources
- ❌ **No raw `new`/`delete`** where avoidable - prefer stack allocation or containers

### Best Practices

- ✅ Use `epoll()` for I/O multiplexing
- ✅ Handle partial reads/writes (non-blocking sockets)
- ✅ Always check return values of system calls
- ✅ Use `LOG()` macro for debugging, not `std::cout`
- ✅ Follow the existing handler pattern for new request types
- ✅ Write tests for new functionality

## HTTP/1.0 Specifics

Per [RFC 1945](https://datatracker.ietf.org/doc/html/rfc1945):

- Support methods: `GET`, `HEAD`, `POST`
- Connection is closed after each request/response (no keep-alive by default)
- Status line format: `HTTP/1.0 <status-code> <reason-phrase>`
- Simple request/response model without chunked transfer encoding

## CGI Implementation

Per [RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875):

- CGI scripts are executed via `fork()` + `exec()`
- Environment variables: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `PATH_INFO`, `SCRIPT_NAME`, etc.
- Request body is passed via stdin to the CGI script
- Response is read from stdout
- Pipes must be monitored via epoll for non-blocking I/O

## References

- [RFC 1945 - HTTP/1.0](https://datatracker.ietf.org/doc/html/rfc1945)
- [RFC 3875 - CGI/1.1 Specification](https://datatracker.ietf.org/doc/html/rfc3875)
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md) - Configuration guide
- [docs/config-schema.json](docs/config-schema.json) - Configuration schema
