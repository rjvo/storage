include(../CMakeTestServer.txt)
include_directories(../../include
                    ../socket_read_write_lib)

add_executable(rmc cmdline.c)

target_link_libraries (rmc LINK_PUBLIC ROjal_MQTT ROjal_MQTT_SOCKET_IF)