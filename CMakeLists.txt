cmake_minimum_required(VERSION 3.10)
project(server C)

set(CMAKE_C_STANDARD 99)

add_executable(server main.c)

if(WIN32)
    target_link_libraries(server wsock32 ws2_32)
endif()