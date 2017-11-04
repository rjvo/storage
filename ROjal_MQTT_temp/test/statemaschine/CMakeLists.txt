include(../CMakeTestServer.txt)
include_directories(../unity
                    ../../include)

add_executable(statemaschine_connect_a test_statemaschine_connect_a.c)
target_link_libraries (statemaschine_connect_a LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschineConnect ${EXECUTABLE_OUTPUT_PATH}/statemaschine_connect_a)

add_executable(statemaschine_connect_b test_statemaschine_connect_a.c)
target_link_libraries (statemaschine_connect_b LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschineConnect ${EXECUTABLE_OUTPUT_PATH}/statemaschine_connect_b)

add_executable(statemaschine_connect_keepalive test_statemaschine_connect_keepalive.c)
target_link_libraries (statemaschine_connect_keepalive LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschineKeepalive ${EXECUTABLE_OUTPUT_PATH}/statemaschine_connect_keepalive)

add_executable(statemaschine_publish test_statemaschine_publish.c)
target_link_libraries (statemaschine_publish LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschinePub ${EXECUTABLE_OUTPUT_PATH}/statemaschine_publish)

add_executable(statemaschine_subscribe test_statemaschine_subscribe.c)
target_link_libraries (statemaschine_subscribe LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschineSub ${EXECUTABLE_OUTPUT_PATH}/statemaschine_subscribe)

add_executable(statemaschine_pubsub test_statemaschine_pubsub.c)
target_link_libraries (statemaschine_pubsub LINK_PUBLIC unity ROjal_MQTT HELP)
add_test(StateMaschinePubSub ${EXECUTABLE_OUTPUT_PATH}/statemaschine_pubsub)
