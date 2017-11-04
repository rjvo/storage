include(../CMakeTestServer.txt)
include_directories(../../include)

add_executable(empty empty.c)

target_link_libraries (empty LINK_PUBLIC ROjal_MQTT)