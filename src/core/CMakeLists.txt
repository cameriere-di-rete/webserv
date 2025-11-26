set(CORE_SOURCES
  Connection.cpp
  Server.cpp
  ServerManager.cpp
)

# Register sources with global list for Makefile generator (relative paths)
foreach(f IN LISTS CORE_SOURCES)
  set(_rel "src/core/${f}")
  set_property(GLOBAL APPEND PROPERTY ALL_SOURCES "${_rel}")
endforeach()

add_library(webserv_core STATIC
  ${CORE_SOURCES}
)

target_include_directories(webserv_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(webserv_core PUBLIC webserv_http webserv_utils)

target_compile_options(webserv_core PRIVATE -Wall -Wextra -Werror)
target_compile_features(webserv_core PUBLIC cxx_std_98)
