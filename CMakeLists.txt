cmake_minimum_required(VERSION 3.10)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hft_sim)

# Find the required packages
# find_package(Threads REQUIRED)
# find_package(OpenSSL REQUIRED)

# Add the source files
set(SOURCES
	main.cpp
	src/OrderBook.cpp
	src/Order.cpp
	# src/Trade.cpp
	# src/MarketDataFeed.cpp
	# src/ExecutionEngine.cpp
)

# Include directories for the project
include_directories(
	${CMAKE_SOURCE_DIR}/include
	# ${CMAKE_SOURCE_DIR}/src
)

# Add the executable target
add_executable(hft_sim ${SOURCES})

# Link the required libraries
# target_link_libraries(hft_sim Threads::Threads OpenSSL::SSL OpenSSL::Crypto)

# Optionally, you can set the output directory for the executable
set_target_properties(hft_sim PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)