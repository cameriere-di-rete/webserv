set(HANDLER_SOURCES
  EchoHandler.cpp
  FileHandler.cpp
  RedirectHandler.cpp
)

foreach(f IN LISTS HANDLER_SOURCES)
  set(_rel "src/handlers/${f}")
  set_property(GLOBAL APPEND PROPERTY ALL_SOURCES "${_rel}")
endforeach()

add_library(webserv_handlers STATIC ${HANDLER_SOURCES})
target_include_directories(webserv_handlers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(webserv_handlers PRIVATE -Wall -Wextra -Werror)
target_compile_features(webserv_handlers PUBLIC cxx_std_98)
