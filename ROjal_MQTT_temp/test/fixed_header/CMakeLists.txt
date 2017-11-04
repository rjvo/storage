include_directories(../unity
                    ../../include)

add_executable(fixed_header_encode_tests test_mqtt_fixed_header_encode.c)
target_link_libraries (fixed_header_encode_tests LINK_PUBLIC unity ROjal_MQTT)
add_test(EncodeFixedHeaders ${EXECUTABLE_OUTPUT_PATH}/fixed_header_encode_tests)

add_executable(fixed_header_decode_tests test_mqtt_fixed_header_decode.c)
target_link_libraries (fixed_header_decode_tests LINK_PUBLIC unity ROjal_MQTT)
add_test(DecodeFixedHeaders ${EXECUTABLE_OUTPUT_PATH}/fixed_header_decode_tests)
