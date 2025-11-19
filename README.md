# webserv

This is when you finally understand why URLs start with HTTP

https://datatracker.ietf.org/doc/html/rfc1945

## Building

The project uses CMake as the build system with a Makefile wrapper for convenience.

### Quick Start

```bash
make        # Build the project
make clean  # Remove build artifacts
make fclean # Remove all generated files
make re     # Rebuild from scratch
```

### Build Options

Set the log level during build:
```bash
make LOG_LEVEL=0  # 0=DEBUG, 1=INFO, 2=ERROR
```

### Build System

- **CMakeLists.txt**: CMake configuration file
- **Makefile**: Wrapper that invokes CMake (maintains original Makefile interface)
- **Makefile.legacy**: Original Makefile (kept for reference)

The wrapper Makefile provides the same interface as the original Makefile, but uses CMake under the hood. All builds happen in the `build/` directory, and the final executable is copied to the project root.