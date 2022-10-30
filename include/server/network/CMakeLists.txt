add_executable(protocol_test protocol_test.cc)

add_test(NAME protocol_test COMMAND protocol_test)
target_include_directories(protocol_test PUBLIC ${NETWORK_INCLUDE_DIR})
