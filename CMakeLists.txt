cmake_minimum_required(VERSION 3.15)

project(webserv LANGUAGES CXX)

# Project must compile as C++98 for the product code. Tests (GTest) will be built
# separately with C++11 to satisfy their requirements.
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)

include(GNUInstallDirs)

# Use per-target compile options; apply common warnings consistently.
set(COMMON_WARNINGS -Wall -Wextra -Werror)

# Optionally enable coverage instrumentation (uses clang/llvm's coverage tools)
option(ENABLE_COVERAGE "Enable coverage instrumentation (clang/llvm)" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Enabling clang/llvm coverage instrumentation flags")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-instr-generate")
    else()
        message(WARNING "ENABLE_COVERAGE requested but compiler is not Clang. Coverage flags not added.")
    endif()
endif()

enable_testing()

# Add submodules (each subdir creates a library target and registers its .cpp files
# with the global property ALL_SOURCES so the Makefile generator can collect them).
# Make headers under src/ visible to all targets to simplify includes during transition
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/http
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/handlers
    ${CMAKE_SOURCE_DIR}/src/utils
)

add_subdirectory(src/http)
add_subdirectory(src/utils)
add_subdirectory(src/config)
add_subdirectory(src/handlers)
add_subdirectory(src/core)

# Create the final executable and link logical libraries
add_executable(webserv src/core/main.cpp)
target_link_libraries(webserv PRIVATE webserv_core webserv_http webserv_config webserv_handlers webserv_utils)
target_include_directories(webserv PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(webserv PRIVATE ${COMMON_WARNINGS})

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Collect all sources registered by subdirectories into a semicolon-separated list
get_property(ALL_SOURCES GLOBAL PROPERTY ALL_SOURCES)
if(NOT ALL_SOURCES)
    message(FATAL_ERROR "No sources registered. Ensure src/*/CMakeLists.txt call set_property(GLOBAL APPEND PROPERTY ALL_SOURCES ...)")
endif()

# Write the sources list to a file for the Makefile generator (semicolon-separated)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sources.txt" "${ALL_SOURCES}")

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
