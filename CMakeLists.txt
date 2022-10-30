cmake_minimum_required(VERSION 3.5)
project(network_server)

set(CMAKE_CXX_STANDARD 17)

set(NETWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(network_server main.cc)
include_directories(${NETWORK_INCLUDE_DIR})

enable_testing()
add_subdirectory(${NETWORK_INCLUDE_DIR}/server/network)
