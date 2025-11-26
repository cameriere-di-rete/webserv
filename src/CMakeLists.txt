## src/CMakeLists.txt
# Builds the main library (`webserv_lib`) from the project's source files
# and the `webserv` executable that links to it.

if(NOT DEFINED SOURCES)
  message(FATAL_ERROR "SOURCES variable is required by src/CMakeLists.txt but was not set in the parent CMakeLists.txt")
endif()

# Remove main.cpp from library sources (it will be the executable entry point)
list(REMOVE_ITEM SOURCES main.cpp)

# Convert names in SOURCES (which are relative filenames) to absolute paths under the project root
set(WEBERV_SRC_FILES)
foreach(f IN LISTS SOURCES)
  list(APPEND WEBERV_SRC_FILES "${CMAKE_SOURCE_DIR}/${f}")
endforeach()

add_library(webserv_lib STATIC ${WEBERV_SRC_FILES})

target_include_directories(webserv_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/http
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/handlers
)

target_compile_features(webserv_lib PUBLIC cxx_std_98)

# Create executable and link the library
add_executable(webserv ${CMAKE_SOURCE_DIR}/src/core/main.cpp)
target_link_libraries(webserv PRIVATE webserv_lib)
target_include_directories(webserv PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/http ${CMAKE_SOURCE_DIR}/src/config ${CMAKE_SOURCE_DIR}/src/utils ${CMAKE_SOURCE_DIR}/src/handlers)

# Apply historical compiler warning flags to targets if global flags are not preferred
target_compile_options(webserv_lib PRIVATE -Wall -Wextra -Werror)
target_compile_options(webserv PRIVATE -Wall -Wextra -Werror)
