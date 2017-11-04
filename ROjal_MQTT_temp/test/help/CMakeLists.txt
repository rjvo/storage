include(../CMakeTestServer.txt)
include_directories(../unity
                    ../../include)

add_library(HELP STATIC help.c)
TARGET_LINK_LIBRARIES(HELP)