# webserv

This is when you finally understand why URLs start with HTTP

https://datatracker.ietf.org/doc/html/rfc1945

## Building

The project uses **CMake** as the source of truth, with a generated **Makefile** for users without CMake.

### Using Makefile (For users without CMake)

The Makefile is automatically generated from CMake and committed to the repository:

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

### Using CMake (For development)

For developers who want to modify the build system:

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

### Regenerating the Makefile

When you modify `CMakeLists.txt`, regenerate the Makefile:

```bash
cmake -B build
cmake --build build --target generate-makefile
```

Then commit the updated Makefile.

### Build System Files

- **CMakeLists.txt**: Source of truth for the build system
- **Makefile**: Auto-generated from CMakeLists.txt (committed for users without CMake)
- **Makefile.in**: Template used to generate the Makefile
- **generate_makefile.cmake**: Script that generates the Makefile from CMakeLists.txt

The Makefile is automatically generated from CMakeLists.txt, so there's no risk of them getting out of sync.