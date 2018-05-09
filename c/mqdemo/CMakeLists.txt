cmake_minimum_required(VERSION 3.10)
project(mqdemo C)

set(CMAKE_C_STANDARD 11)

add_executable(server server.c utils.c utils.h)
target_link_libraries(server rabbitmq)
