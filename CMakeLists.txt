cmake_minimum_required(VERSION 2.8)
enable_testing()
project(vmars)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

find_package(Threads)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dep/aeron-1.5.1/aeron-client/src/main/cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/dep/aeron-1.5.1/aeron-client/src/main/cpp)

set(SOURCE
    src/packet.c
    src/counter_handler.c
    src/common.c
    src/jodie_client.c
    src/ringbuffer.cpp)

set(HEADERS
    src/atomic.h
    src/common.h
    src/packet.h
    src/counter_handler.h
    src/jodie_client.h
    src/ringbuffer.h)

add_executable(
    vmars
    src/main.c
    ${SOURCE} ${HEADERS})

target_link_libraries(vmars aeron_client ${CMAKE_THREAD_LIBS_INIT})

add_executable(directio src/directio.c)
