cmake_minimum_required(VERSION 3.14)
project(MyTests)

enable_testing()

# Try to find local GTest installation first, fallback to FetchContent
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found locally, fetching from GitHub...")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
else()
  message(STATUS "Using locally installed GTest")
endif()

include_directories(${CMAKE_SOURCE_DIR})

# Build tests by linking against the library target `webserv_lib` so we don't recompile sources
add_executable(runTests test_main.cpp ../src/utils/utils_test.cpp)
target_link_libraries(runTests PRIVATE GTest::gtest_main webserv_lib pthread)

# Ensure the test target can find headers moved into `src/*`
target_include_directories(runTests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/http ${CMAKE_SOURCE_DIR}/src/config ${CMAKE_SOURCE_DIR}/src/utils ${CMAKE_SOURCE_DIR}/src/handlers ${CMAKE_SOURCE_DIR}/include)

target_compile_options(runTests PRIVATE -Wall -Wextra -Werror)
target_compile_features(runTests PRIVATE cxx_std_11)

add_test(NAME MyTest COMMAND runTests)
