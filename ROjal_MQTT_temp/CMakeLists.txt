cmake_minimum_required(VERSION 3.0)

project("ROjal_MQTT_Client" VERSION 0.0.1 LANGUAGES C)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include(CTest)

add_definitions("-DBUILD_DEFAULT_C_LIBS=1")

set(CMAKE_C_FLAGS "-DDEBUG -std=gnu11 -O0 -fno-strict-aliasing -g -Wall -W -fstack-protector-all -Wextra -ftrapv -fstack-usage")

# No debug and size optimized
# set(CMAKE_C_FLAGS "-DNoDEBUG -std=gnu11 -Os -g -Wall")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-Map=out.map")

add_subdirectory(src)
add_subdirectory(test)
