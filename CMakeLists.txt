cmake_minimum_required(VERSION 3.5)
enable_testing()

project(packet_mmap)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g")
find_package(Threads)

set(SOURCE
    src/packet.c
    src/fixparser.c
    src/spsc_rb.c
    src/simpleboyermoore.c
    src/fix.c)

set(HEADERS
    src/packet.h
    src/fixparser.h
    src/simpleboyermoore.h
    src/spsc_rb.h
    src/fix.h)

add_executable(
    packet
    src/main.c
    ${SOURCE} ${HEADERS})
target_link_libraries(packet ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(directio src/directio.c)

#Tests
add_executable(simpleboyermoore_test src/simpleboyermoore.h src/simpleboyermoore.c test/simpleboyermoore_test.c)
target_include_directories(simpleboyermoore_test PRIVATE src)

add_executable(
    packet_extract_fix_messages_test
    test/packet_extract_fix_messages_test.c
    ${SOURCE} ${HEADERS})
target_include_directories(packet_extract_fix_messages_test PRIVATE src)
target_link_libraries(packet_extract_fix_messages_test ${CMAKE_THREAD_LIBS_INIT} rt)

add_executable(
    spsc_rb_test src/spsc_rb.h src/spsc_rb.c test/spsc_rb_test.c
)
target_include_directories(spsc_rb_test PRIVATE src)
target_link_libraries(spsc_rb_test rt)

add_test(packet_exctract_fix_messages packet_extract_fix_messages_test)
add_test(spsc_rb spsc_rb_test)
add_test(simpleboyermoore simpleboyermoore_test)
