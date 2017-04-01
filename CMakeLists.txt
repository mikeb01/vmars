cmake_minimum_required(VERSION 3.5)
project(packet_mmap)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
find_package(Threads)

add_executable(packet packet.c)
add_executable(directio directio.c)
add_executable(fanout fanout.c)

target_link_libraries(packet ${CMAKE_THREAD_LIBS_INIT})