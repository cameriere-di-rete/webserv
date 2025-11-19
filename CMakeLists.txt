cmake_minimum_required(VERSION 3.10)
project(webserv)

# Set C++ standard to C++98 to match original Makefile
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags matching original Makefile
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Optional LOG_LEVEL support
# Usage: cmake -DLOG_LEVEL=0 .. (0=DEBUG, 1=INFO, 2=ERROR)
if(DEFINED LOG_LEVEL)
    add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})
endif()

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
