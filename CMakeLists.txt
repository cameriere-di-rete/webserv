cmake_minimum_required(VERSION 3.10)
project(webserv)

# Set C++ standard to C++98 to match original Makefile
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags matching original Makefile
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Source files from original Makefile
set(SOURCES
    main.cpp
    BlockNode.cpp
    Body.cpp
    Config.cpp
    Connection.cpp
    DirectiveNode.cpp
    Header.cpp
    HttpMethod.cpp
    HttpStatus.cpp
    Location.cpp
    Logger.cpp
    Message.cpp
    Request.cpp
    RequestLine.cpp
    Response.cpp
    Server.cpp
    ServerManager.cpp
    StatusLine.cpp
    utils.cpp
)

# Create executable
add_executable(webserv ${SOURCES})

# Write sources list to a temporary file for the generator script
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sources.txt" "${SOURCES}")

# Custom target to generate standalone Makefile
# This creates a simple Makefile that can be committed and used without CMake
add_custom_target(generate-makefile
    COMMAND ${CMAKE_COMMAND}
        -DSOURCES_FILE="${CMAKE_CURRENT_BINARY_DIR}/sources.txt"
        -DINPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in"
        -DOUTPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/generate_makefile.cmake"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating standalone Makefile from CMakeLists.txt"
)
