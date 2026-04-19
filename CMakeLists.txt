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

find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)

find_program(GCOV_EXECUTABLE NAMES gcov-13 gcov REQUIRED)

add_library(Core STATIC
  core/Exchange.cpp
  core/ExchangeLogger.cpp
  core/FieldMap.cpp
  core/OrderBook.cpp
  core/ThreadPool.cpp
)

target_compile_options(Core PRIVATE ${WARNINGS})

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

# add_executable(FastLittleMarket ...)
# target_link_libraries(FastLittleMarket PRIVATE Core)

option(BUILD_TESTS "Build unit tests" ON)
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
    test/test_FixAlloc.cpp
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

  include(GoogleTest)
  gtest_discover_tests(tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE PRE_TEST
  )

  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND gcovr
    -r ${CMAKE_SOURCE_DIR}
    --gcov-executable ${GCOV_EXECUTABLE}
    --filter ${CMAKE_SOURCE_DIR}/core
    --exclude ${CMAKE_SOURCE_DIR}/test
    --exclude "/usr/include/.*"
    --exclude ".*external.*"
    --gcov-ignore-parse-errors=negative_hits.warn
    --txt -o ${CMAKE_SOURCE_DIR}/coverage/coverage.txt
    --html --html-details
    -o ${CMAKE_SOURCE_DIR}/coverage/coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report"
  )
endif()
