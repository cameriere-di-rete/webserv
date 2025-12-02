# webserv

This is when you finally understand why URLs start with HTTP

https://datatracker.ietf.org/doc/html/rfc1945

## Development

### Code Formatting

This repository uses `clang-format` for consistent code style. To format all C++ files in a pull request, simply comment `/format` on the PR. The GitHub Action will automatically format the code and push the changes to your branch.

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

### Using CMake (For development)

For developers who want to modify the build system:

```bash
mkdir build && cd build
cmake ..
make
```

## Running

Run the server with a configuration file:

```bash
./webserv [config_file]
```

Set the log level at runtime:
```bash
./webserv -l:0           # Use default config with DEBUG level
./webserv -l:1 my.conf   # Use custom config with INFO level
```

Log levels: 0=DEBUG, 1=INFO, 2=ERROR

If no config file is specified, the default is `conf/default.conf`.

For detailed configuration documentation, see [docs/CONFIGURATION.md](docs/CONFIGURATION.md).

### Regenerating the Makefile

The Makefile is generated from `CMakeLists.txt` using the `generate-makefile` target.

Automatic regeneration: the generated `Makefile` now embeds a small stamp-based rule so
`make` will automatically regenerate the top-level `Makefile` when important CMake files
change (for example `CMakeLists.txt`, `Makefile.in`, and other `*.cmake` files). The stamp file used is
`build/.cmake_stamp`.

**Note:** Automatic regeneration only works if CMake is installed and available in your environment.
If CMake is not available, a warning is displayed but the build continues using the committed Makefile.

Manual regeneration options:

- Run the CMake generator directly:

```bash
cmake -B build
cmake --build build --target generate-makefile
```

- Or use the convenience make target (embedded in the generated `Makefile`):

```bash
make regenerate        # alias for regenerate-stamp
make regenerate-stamp  # force regeneration via the stamp target
```

After regeneration, if you want to keep the generated `Makefile` in the repo, commit the updated file.

### Build System Files

- **CMakeLists.txt**: Source of truth for the build system
- **Makefile**: Auto-generated from CMakeLists.txt (committed for users without CMake)
- **cmake/generate_makefile.cmake**: Script that generates the Makefile from CMakeLists.txt
- **cmake/Makefile.in**: Template used to generate the top-level `Makefile`

The Makefile is automatically generated from CMakeLists.txt, so there's no risk of them getting out of sync.
