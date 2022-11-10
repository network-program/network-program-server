project(jsoncpp)

add_library(jsoncpp
        ${CMAKE_CURRENT_SOURCE_DIR}/json/json_reader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/json/json_value.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/json/json_writer.cpp
        )

target_include_directories(jsoncpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
