cmake_minimum_required(VERSION 3.28.3)

project(Result VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_library(Logger INTERFACE)
target_include_directories(Logger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(LoggerExample exampels/LoggerExample.cpp)
target_link_libraries(LoggerExample PRIVATE Logger)




include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(LoggerTest tests/LoggerTest.cpp)
target_include_directories(LoggerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/mocks)
target_link_libraries(LoggerTest PRIVATE Logger GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(LoggerTest)
