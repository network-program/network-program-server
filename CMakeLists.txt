cmake_minimum_required(VERSION 3.5)
project(network_server)

set(CMAKE_CXX_STANDARD 17)

set(NETWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

add_subdirectory("third_party/jsoncpp" EXCLUDE_FROM_ALL)

include_directories(${NETWORK_INCLUDE_DIR})

add_executable(network_server main.cc)

add_executable(chat_server "${CMAKE_CURRENT_SOURCE_DIR}/cinclude/server/network/request_msg_test.cc")
target_link_libraries(chat_server PUBLIC jsoncpp pthread)
target_compile_options(chat_server PRIVATE -fopenmp)

enable_testing()
add_subdirectory(${NETWORK_INCLUDE_DIR}/server/network)
