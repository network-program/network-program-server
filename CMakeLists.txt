cmake_minimum_required(VERSION 3.5)
project(chat_server)

set(CMAKE_CXX_STANDARD 17)

set(NETWORK_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsoncpp" EXCLUDE_FROM_ALL)

include_directories(${NETWORK_INCLUDE_DIR})

add_executable(chat_server main.cc)
target_link_libraries(chat_server PUBLIC jsoncpp pthread)

if (UNIX AND NOT APPLE)
  target_compile_options(chat_server PRIVATE -fopenmp)
endif()

enable_testing()
add_subdirectory(${NETWORK_INCLUDE_DIR}/server/protocol)
