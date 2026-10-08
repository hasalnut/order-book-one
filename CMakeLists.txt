cmake_minimum_required(VERSION 3.24)
project(OrderBookOne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ORDERBOOK_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(ORDERBOOK_BUILD_TESTS "Build unit tests" ON)
option(ORDERBOOK_BUILD_BENCH "Build benchmarks" ON)

# Sanitizers are applied globally (before dependencies are fetched) so that
# GoogleTest and Google Benchmark are instrumented the same way as our code.
if(ORDERBOOK_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Strict warnings for our own targets only (not third-party code).
add_library(orderbook_warnings INTERFACE)
target_compile_options(orderbook_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)

# ---- Dependencies ----
include(FetchContent)

if(ORDERBOOK_BUILD_TESTS)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

if(ORDERBOOK_BUILD_BENCH)
  FetchContent_Declare(benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
    GIT_SHALLOW TRUE)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(benchmark)
endif()

# ---- Engine library ----
add_library(orderbook
  engine/src/placeholder.cpp)
target_include_directories(orderbook PUBLIC engine/include)
target_link_libraries(orderbook PRIVATE orderbook_warnings)

# ---- Tests ----
if(ORDERBOOK_BUILD_TESTS)
  enable_testing()
  add_executable(orderbook_tests
    tests/smoke_test.cpp)
  target_link_libraries(orderbook_tests PRIVATE orderbook orderbook_warnings GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(orderbook_tests)
endif()

# ---- Benchmarks ----
if(ORDERBOOK_BUILD_BENCH)
  add_executable(orderbook_bench
    bench/smoke_bench.cpp)
  target_link_libraries(orderbook_bench PRIVATE orderbook orderbook_warnings benchmark::benchmark_main)
endif()
