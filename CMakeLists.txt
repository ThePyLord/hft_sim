cmake_minimum_required(VERSION 3.14)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexperimental-library")

project(hft_sim)

# Find the required packages
#find_package(Threads REQUIRED)
# find_package(OpenSSL REQUIRED)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

# Net library
set(NET_SOURCES
    src/net/UdpReliable.cpp
    src/net/Protocol.cpp
    src/net/Crc32c.cpp
    src/net/ReorderingBuffer.cpp
)
add_library(hsnet STATIC ${NET_SOURCES})
target_include_directories(hsnet PUBLIC ${CMAKE_SOURCE_DIR}/include/net)

# Add the source files for the main executable
set(SOURCES
    main.cpp
    src/OrderBook.cpp
    src/Order.cpp
    util/Logger.cpp
    # src/LockFreeQueue.cpp
    # src/Trade.cpp
    # src/MarketDataFeed.cpp
    # src/ExecutionEngine.cpp
)

# Include directories for the project
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/util
)

# Add the main executable
add_executable(hft_sim ${SOURCES})

target_link_libraries(hft_sim PRIVATE hsnet)

# Enable testing with CTest
enable_testing()

# Set output directories for the main executable
set_target_properties(hft_sim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
# Set the default runtime output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For test executables, override to bin/tests
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/tests)

# Logger test executable
add_executable(test_logger util/test_logger.cpp util/Logger.cpp)
set_target_properties(test_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_include_directories(test_logger PRIVATE ${CMAKE_SOURCE_DIR}/util ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_logger PRIVATE gtest gtest_main)
add_test(NAME LoggerTest COMMAND ${CMAKE_BINARY_DIR}/bin/test_logger)

# Test executables in the tests directory
set(TEST_SOURCES
        tests/test_cancel_order.cpp
        tests/test_full_orders.cpp
        tests/test_lockfree_queue.cpp
        tests/test_crc32c.cpp
        tests/test_reordering_buffer.cpp
        tests/test_udp_transport.cpp
)

# Function to add a Google Test executable (commented out for now)
function(add_gtest_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} src/OrderBook.cpp src/Order.cpp util/Logger.cpp)
     set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/util)
    target_link_libraries(${TEST_NAME} PRIVATE gtest gtest_main hsnet)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_OUTPUT_DIR}/${TEST_NAME})
    message("Writing test executable: ${TEST_NAME} with source: ${TEST_SOURCE} to ${TEST_OUTPUT_DIR}")
endfunction()

add_gtest_test(test_full_orders tests/test_full_orders.cpp)
add_gtest_test(test_cancel_order tests/test_cancel_order.cpp)
add_gtest_test(test_crc32c tests/test_crc32c.cpp)
add_gtest_test(test_reordering_buffer tests/test_reordering_buffer.cpp)


#foreach(TEST_SRC ${TEST_SOURCES})
#    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
#    add_executable(${TEST_NAME} ${TEST_SRC} src/OrderBook.cpp src/Order.cpp util/Logger.cpp)
#    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
#    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/util)
#    target_link_libraries(${TEST_NAME} PRIVATE gtest gtest_main hsnet)
#    add_test(NAME ${TEST_NAME} COMMAND ${TEST_OUTPUT_DIR}/${TEST_NAME})
#endforeach()

# Dedicated UDP transport test
add_executable(test_udp_transport tests/test_udp_transport.cpp)
set_target_properties(test_udp_transport PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
target_include_directories(test_udp_transport PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_udp_transport PRIVATE gtest gtest_main hsnet)
add_test(NAME UdpTransportTest COMMAND ${TEST_OUTPUT_DIR}/test_udp_transport)

# Non-test demo
add_executable(feed_pub examples/feed_publisher_demo.cpp)
target_include_directories(feed_pub PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(feed_pub PRIVATE hsnet)

add_executable(feed_sub examples/feed_subscriber_demo.cpp)
target_include_directories(feed_sub PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(feed_sub PRIVATE hsnet)
