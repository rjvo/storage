include(../CMakeTestServer.txt)
include_directories(../unity
                    ../../include
                    ../socket_read_write_lib
                    ../help)

add_executable(prod_test_conn prod_test_connect.c prod_help.c )
target_link_libraries (prod_test_conn LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(PRODTestConnect ${EXECUTABLE_OUTPUT_PATH}/prod_test_conn)

add_executable(prod_test_pub prod_test_publish.c prod_help.c )
target_link_libraries (prod_test_pub LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(PRODTestPublish ${EXECUTABLE_OUTPUT_PATH}/prod_test_pub)

add_executable(prod_test_sub prod_test_subscribe.c prod_help.c )
target_link_libraries (prod_test_sub LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(PRODTestSubscribe ${EXECUTABLE_OUTPUT_PATH}/prod_test_sub)

add_executable(prod_test_pubsub prod_test_pubsub.c prod_help.c )
target_link_libraries (prod_test_pubsub LINK_PUBLIC unity ROjal_MQTT ROjal_MQTT_SOCKET_IF HELP)
add_test(PRODTestPubSub ${EXECUTABLE_OUTPUT_PATH}/prod_test_pubsub)