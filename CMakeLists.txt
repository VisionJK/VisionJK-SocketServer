cmake_minimum_required (VERSION 3.14.1)
project (visionServer)
add_executable(visionNetwork main.c server_functions.c packet_functions.c functions.h socket_functions.c)
target_link_libraries(visionNetwork "wsock32")