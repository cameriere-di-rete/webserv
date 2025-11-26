set(CONFIG_SOURCES
  BlockNode.cpp
  Config.cpp
  DirectiveNode.cpp
  Location.cpp
)

foreach(f IN LISTS CONFIG_SOURCES)
  set(_rel "src/config/${f}")
  set_property(GLOBAL APPEND PROPERTY ALL_SOURCES "${_rel}")
endforeach()

add_library(webserv_config STATIC ${CONFIG_SOURCES})
target_include_directories(webserv_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(webserv_config PRIVATE -Wall -Wextra -Werror)
target_compile_features(webserv_config PUBLIC cxx_std_98)
