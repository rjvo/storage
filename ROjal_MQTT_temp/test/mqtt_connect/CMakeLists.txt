include(../CMakeTestServer.txt)
include_directories(../unity
                    ../../include)

add_executable(simple_raw_connect_tests test_mqtt_connect_simple_raw.c)
target_link_libraries (simple_raw_connect_tests LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(MqttSimpleConnectRaw ${EXECUTABLE_OUTPUT_PATH}/simple_raw_connect_tests)

add_executable(simple_connect_tests test_mqtt_connect_simple_rojal_mqtt.c)
target_link_libraries (simple_connect_tests LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(MqttSimpleConnect ${EXECUTABLE_OUTPUT_PATH}/simple_connect_tests)

add_executable(simple_connect_keepalive_tests test_mqtt_connect_simple_keepalive_rojal_mqtt.c)
target_link_libraries (simple_connect_keepalive_tests LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(MqttSimpleConnectKeepalive ${EXECUTABLE_OUTPUT_PATH}/simple_connect_keepalive_tests)