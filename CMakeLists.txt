cmake_minimum_required(VERSION 3.20)

project(FastLittleMarket VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wpedantic")

# Find dependencies (Ubuntu apt-installed)
find_package(Boost REQUIRED COMPONENTS system headers)
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED) # Ubuntu: libgtest-dev

add_library(Core STATIC
  core/OrderBook.cpp
  core/Exchange.cpp
  core/ThreadPool.cpp
)

target_include_directories(Core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/data
  ${CMAKE_CURRENT_SOURCE_DIR}/matching_engine
  ${CMAKE_CURRENT_SOURCE_DIR}/external/concurrentqueue
  ${CMAKE_CURRENT_SOURCE_DIR}/external/quill
)

target_link_libraries(Core
  Boost::system
  Boost::headers
)

# Main executable
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
  )

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

  include(GoogleTest)
  gtest_discover_tests(tests)

  add_test(NAME FastLittleMarketTests COMMAND tests)
endif()
