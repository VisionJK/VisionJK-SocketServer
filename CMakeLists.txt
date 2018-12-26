cmake_minimum_required (VERSION 2.8.9)
project (visionServer)
add_executable(visionNetwork main.c functions.c functions.h socket_functions.c)
target_link_libraries(visionNetwork "wsock32")