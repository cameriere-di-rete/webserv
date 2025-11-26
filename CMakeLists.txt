cmake_minimum_required(VERSION 3.15)

project(webserv VERSION 0.1 LANGUAGES CXX)

# Keep the project's historical C++ standard (the codebase currently expects C++98)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Recommended: prefer target-based compile options; keep warnings used historically
add_compile_options(-Wall -Wextra -Werror)

# Explicit list of source files (kept for compatibility with existing generator)
set(SOURCES
    src/config/BlockNode.cpp
    src/config/Config.cpp
    src/config/DirectiveNode.cpp
    src/config/Location.cpp
    src/core/Connection.cpp
    src/core/main.cpp
    src/core/Server.cpp
    src/core/ServerManager.cpp
    src/handlers/EchoHandler.cpp
    src/handlers/FileHandler.cpp
    src/handlers/RedirectHandler.cpp
    src/http/Body.cpp
    src/http/Header.cpp
    src/http/HttpMethod.cpp
    src/http/HttpStatus.cpp
    src/http/Message.cpp
    src/http/Request.cpp
    src/http/RequestLine.cpp
    src/http/Response.cpp
    src/http/StatusLine.cpp
    src/utils/file_utils.cpp
    src/utils/Logger.cpp
    src/utils/utils.cpp
)

option(BUILD_TESTS "Build unit tests" ON)

enable_testing()

# Include modular source directory (creates library and executable targets)
add_subdirectory(src)

# Tests remain in `tests/` for now; per-file tests can be added next to sources later
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Keep the standalone Makefile generator for compatibility with prior workflow
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sources.txt"
    COMMAND ${CMAKE_COMMAND} -E echo_append "${SOURCES}" > "${CMAKE_CURRENT_BINARY_DIR}/sources.txt"
    DEPENDS CMakeLists.txt
    COMMENT "Writing sources.txt from SOURCES list"
    VERBATIM
)

add_custom_target(generate-makefile
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sources.txt"
    COMMAND ${CMAKE_COMMAND}
        -DSOURCES_FILE="${CMAKE_CURRENT_BINARY_DIR}/sources.txt"
            -DINPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/cmake/Makefile.in"
            -DOUTPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_makefile.cmake"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating standalone Makefile from CMakeLists.txt"
)
