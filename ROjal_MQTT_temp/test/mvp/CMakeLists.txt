include(../CMakeTestServer.txt)
include_directories(../unity
                    ../../include
                    ../socket_read_write_lib
                    ../help)

add_executable(mvp_test_connect mvp_test_connect.c mvp_help.c)
target_link_libraries (mvp_test_connect LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(MVPTestConnect ${EXECUTABLE_OUTPUT_PATH}/mvp_test_connect)

add_executable(mvp_test_publish mvp_test_publish.c mvp_help.c)
target_link_libraries (mvp_test_publish LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(MVPTestPublish ${EXECUTABLE_OUTPUT_PATH}/mvp_test_publish)

add_executable(mvp_test_subscribe mvp_test_subscribe.c mvp_help.c)
target_link_libraries (mvp_test_subscribe LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(MVPTestSubscribe ${EXECUTABLE_OUTPUT_PATH}/mvp_test_subscribe)

add_executable(mvp_test_pubsub mvp_test_pubsub.c mvp_help.c)
target_link_libraries (mvp_test_pubsub LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(MVPTestPubSub ${EXECUTABLE_OUTPUT_PATH}/mvp_test_pubsub)
