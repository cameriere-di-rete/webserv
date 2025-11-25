cmake_minimum_required(VERSION 3.14)
project(MyTests)

enable_testing()

# Fetch GoogleTest instead of requiring system installation
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(runTests test_main.cpp ../utils_test.cpp ../utils.cpp)
target_link_libraries(runTests GTest::gtest GTest::gtest_main pthread)

target_compile_options(runTests PRIVATE -Wall -Wextra -Werror)
target_compile_features(runTests PRIVATE cxx_std_11)

add_test(NAME MyTest COMMAND runTests)
