set(UTILS_SOURCES
  file_utils.cpp
  Logger.cpp
  utils.cpp
)

foreach(f IN LISTS UTILS_SOURCES)
  set(_rel "src/utils/${f}")
  set_property(GLOBAL APPEND PROPERTY ALL_SOURCES "${_rel}")
endforeach()

add_library(webserv_utils STATIC ${UTILS_SOURCES})
target_include_directories(webserv_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(webserv_utils PRIVATE -Wall -Wextra -Werror)
target_compile_features(webserv_utils PUBLIC cxx_std_98)
