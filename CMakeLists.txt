cmake_minimum_required(VERSION 3.20)
project(FastLittleMarket VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "-O0 -g --coverage" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_COVERAGE "-O0 -g --coverage" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING "" FORCE)

set(WARNINGS -Wall -Wextra -Wpedantic)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Enable when profiling with perf/Hotspot.
# Use with: cmake ... -DBUILD_BENCHMARKS=ON -DPROFILING=ON
option(PROFILING "Enable profiling flags (frame pointers + debug symbols)" OFF)

find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)
find_program(GCOV_EXECUTABLE NAMES gcov-13 gcov REQUIRED)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------

add_library(Core STATIC
  core/Exchange.cpp
  core/ExchangeLogger.cpp
  core/FieldMap.cpp
  core/OrderBook.cpp
  core/ThreadPool.cpp
  core/Parser.cpp
)

set(CORE_COMPILE_FLAGS ${WARNINGS})
if(PROFILING)
  list(APPEND CORE_COMPILE_FLAGS -fno-omit-frame-pointer -g)
endif()

target_compile_options(Core PRIVATE ${CORE_COMPILE_FLAGS})

target_include_directories(Core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/data
  ${CMAKE_CURRENT_SOURCE_DIR}/matching_engine
  ${CMAKE_CURRENT_SOURCE_DIR}/external/concurrentqueue
  ${CMAKE_CURRENT_SOURCE_DIR}/external/quill
  ${CMAKE_CURRENT_SOURCE_DIR}/external/quill/include
)

target_link_libraries(Core
  Boost::system
  Boost::headers
)

target_compile_definitions(Core PUBLIC
  FLM_LOG_DIR="${CMAKE_SOURCE_DIR}/log"
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(BUILD_TESTS)
  enable_testing()

  add_executable(tests
    test/test_Order.cpp
    test/test_OrderBook.cpp
    test/test_PriorityQueueAdapter.cpp
    test/test_Exchange.cpp
    test/test_ThreadPool.cpp
    test/test_GlobalSequencer.cpp
    test/test_ExchangeLogger.cpp
    test/test_Alloc.cpp
    test/test_FieldList.cpp
    test/test_FieldMap.cpp
    test/test_Parser.cpp
    test/test_Builder.cpp
    test/test_Parser_Builder.cpp
  )

  target_compile_options(tests PRIVATE ${WARNINGS})

  target_link_libraries(tests
    Core
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
  )

  target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/matching_engine
  )

  target_compile_definitions(Core PRIVATE FLM_TESTING)
  target_compile_definitions(tests PRIVATE FLM_TESTING)

  if(ENABLE_ASAN)
    target_compile_options(tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(tests PRIVATE -fsanitize=address)
  endif()

  include(GoogleTest)
  gtest_discover_tests(tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE PRE_TEST
  )

  file(WRITE ${CMAKE_BINARY_DIR}/run_coverage.sh
    "#!/bin/sh
  gcovr \\
  -r ${CMAKE_SOURCE_DIR} \\
  --object-directory ${CMAKE_BINARY_DIR} \\
  --gcov-executable ${GCOV_EXECUTABLE} \\
  --filter '.*core/.*' \\
  --exclude '.*test/.*' \\
  --exclude '/usr/include/.*' \\
  --exclude '.*external.*' \\
  --gcov-ignore-parse-errors=negative_hits.warn \\
  --print-summary \\
  --txt -o ${CMAKE_SOURCE_DIR}/coverage/coverage.txt \\
  --html --html-details -o ${CMAKE_SOURCE_DIR}/coverage/coverage.html
")

  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
    COMMAND sh ${CMAKE_BINARY_DIR}/run_coverage.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report"
  )
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

option(BUILD_BENCHMARKS "Build benchmarks" OFF)

if(BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  file(GLOB BENCHMARK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/bench_*.cpp")

  set(BENCH_COMPILE_FLAGS ${WARNINGS} -fno-lto)
  if(PROFILING)
    list(APPEND BENCH_COMPILE_FLAGS -O2 -g -fno-omit-frame-pointer)
  else()
    list(APPEND BENCH_COMPILE_FLAGS -O3)
  endif()

  foreach(BENCH_SRC ${BENCHMARK_SOURCES})
    # bench_Parser.cpp --> bench_Parser
    get_filename_component(BENCH_NAME ${BENCH_SRC} NAME_WE)

    add_executable(${BENCH_NAME} ${BENCH_SRC})

    target_compile_options(${BENCH_NAME} PRIVATE ${BENCH_COMPILE_FLAGS})
    target_link_options(${BENCH_NAME} PRIVATE -fno-lto)

    target_link_libraries(${BENCH_NAME}
      Core
      benchmark::benchmark
      benchmark::benchmark_main
    )

    target_include_directories(${BENCH_NAME} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/core
      ${CMAKE_CURRENT_SOURCE_DIR}/common
      ${CMAKE_CURRENT_SOURCE_DIR}/data
      ${CMAKE_CURRENT_SOURCE_DIR}/matching_engine
    )
  endforeach()
endif()
