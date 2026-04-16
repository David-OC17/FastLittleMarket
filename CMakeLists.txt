cmake_minimum_required(VERSION 3.20)

project(FastLittleMarket VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WARNINGS -Wall -Wextra -Wpedantic)

find_package(Boost REQUIRED COMPONENTS system headers)
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED) # Ubuntu: libgtest-dev

add_library(Core STATIC
  core/OrderBook.cpp
  core/Exchange.cpp
  core/ThreadPool.cpp
  core/ExchangeLogger.cpp
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

# add_executable(FastLittleMarket ...)
# target_link_libraries(FastLittleMarket PRIVATE Core)

# Tests
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

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COVERAGE_FLAGS --coverage -O0 -g)

    target_compile_options(Core PRIVATE ${COVERAGE_FLAGS})
    target_link_options(Core PRIVATE --coverage)

    target_compile_options(tests PRIVATE ${COVERAGE_FLAGS})
    target_link_options(tests PRIVATE --coverage)
  endif()

  target_compile_definitions(Core PRIVATE TEST_LOGGING)

  include(GoogleTest)
  gtest_discover_tests(tests)

  add_test(NAME FastLittleMarketTests COMMAND tests)

  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND}

    COMMAND gcovr
    -r ..
    --gcov-executable gcov-13
    --filter ../core
    --exclude ../test
    --exclude '/usr/include/.*'
    --exclude '.*external.*'
    --gcov-ignore-parse-errors=negative_hits.warn
    --html --html-details
    -o ../coverage/coverage.html

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()
