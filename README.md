# webserv

This is when you finally understand why URLs start with HTTP

https://datatracker.ietf.org/doc/html/rfc1945

## Building

The project supports two build systems: **Makefile** (default) and **CMake** (optional).

### Using Makefile (Default)

For users with only `make` available:

```bash
make        # Build the project
make clean  # Remove build artifacts
make fclean # Remove all generated files
make re     # Rebuild from scratch
```

Set the log level during build:
```bash
make LOG_LEVEL=0  # 0=DEBUG, 1=INFO, 2=ERROR
```

### Using CMake (Optional)

For users who prefer CMake or need IDE integration:

```bash
mkdir build && cd build
cmake ..
make
```

Or with log level:
```bash
cmake -DLOG_LEVEL=0 ..
make
```

### Build System Files

- **Makefile**: Standalone Makefile for direct compilation
- **CMakeLists.txt**: CMake configuration file (kept in sync with Makefile)

Both build systems produce identical binaries and support the same build options.