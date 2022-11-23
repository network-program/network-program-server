add_executable(protocol_test protocol_test.cc)

add_test(NAME protocol_test COMMAND protocol_test)
target_include_directories(protocol_test PUBLIC ${NETWORK_INCLUDE_DIR})

add_executable(http_protocol_test http_protocol_test.cc)

add_test(NAME http_protocol_test COMMAND http_protocol_test)
target_include_directories(http_protocol_test PUBLIC ${NETWORK_INCLUDE_DIR})
