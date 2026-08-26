cmake_minimum_required(VERSION 3.15...4.4)

project(SCSP_QUEUE VERSION 0.1
                  DESCRIPTION "Single producer - single consumer queue implementation"
                  LANGUAGES CXX)

include(FetchContent)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        main
)

# turn off testing of google benchmark
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googlebench
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(googlebench)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()


add_subdirectory(gtest)
add_subdirectory(benchmarks)

add_executable(example example.cpp)