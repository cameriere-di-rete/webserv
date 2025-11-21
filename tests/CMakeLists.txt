cmake_minimum_required(VERSION 3.10)
project(MyTests)

enable_testing()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})

add_executable(runTests test_main.cpp test_utils.cpp ../utils.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread)

target_compile_options(runTests PRIVATE -Wall -Wextra -Werror)
target_compile_features(runTests PRIVATE cxx_std_11)

add_test(NAME MyTest COMMAND runTests)
