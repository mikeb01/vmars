cmake_minimum_required(VERSION 3.5)
project(packet_mmap)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
find_package(Threads)

add_executable(packet src/packet.c)
add_executable(directio src/directio.c)

add_executable(simpleboyermoore_test src/simpleboyermoore.h test/simpleboyermoore_test.c)

target_include_directories(simpleboyermoore_test PRIVATE src)

target_link_libraries(packet ${CMAKE_THREAD_LIBS_INIT})