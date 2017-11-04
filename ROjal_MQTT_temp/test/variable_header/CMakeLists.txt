include_directories(../unity
                    ../../include)

add_executable(variable_header_decode_tests test_mqtt_variable_header_decode.c)
target_link_libraries (variable_header_decode_tests LINK_PUBLIC unity ROjal_MQTT)
add_test(DecodeVariableHeaders ${EXECUTABLE_OUTPUT_PATH}/variable_header_decode_tests)
