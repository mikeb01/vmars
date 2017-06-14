cmake_minimum_required(VERSION 3.5)
enable_testing()
project(vmars)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g")
find_package(Threads)

##########################################################
# HdrHistogram usage - use MD5 as means to identify snapshot

ExternalProject_Add(
    hdr_histogram
    URL ${CMAKE_CURRENT_SOURCE_DIR}/lib/HdrHistogram_c-0.9.6.zip
    URL_MD5 39007a9f2770b7cc485c3e90d36145f7
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(hdr_histogram source_dir)
set(HDRHISTOGRAM_SOURCE_DIR ${source_dir})
ExternalProject_Get_Property(hdr_histogram binary_dir)
set(HDRHISTOGRAM_BINARY_DIR ${binary_dir})

set(HDRHISTOGRAM_LIBS
    ${HDRHISTOGRAM_BINARY_DIR}/src/${CMAKE_STATIC_LIBRARY_PREFIX}hdr_histogram_static${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

include_directories(${HDRHISTOGRAM_SOURCE_DIR}/src)
link_libraries(${HDRHISTOGRAM_LIBS} ${CMAKE_THREAD_LIBS_INIT} rt m)

set(SOURCE
    src/packet.c
    src/fixparser.c
    src/spsc_rb.c
    src/simpleboyermoore.c
    src/fix.c
    src/latency_handler.c
    src/counter_handler.c
    src/JodieClient.c)

set(HEADERS
    src/common.h
    src/packet.h
    src/fixparser.h
    src/simpleboyermoore.h
    src/spsc_rb.h
    src/fix.h
    src/latency_handler.h
    src/counter_handler.h
    src/khash.h
    src/JodieClient.h)

add_executable(
    vmars
    src/main.c
    ${SOURCE} ${HEADERS})

add_dependencies(vmars hdr_histogram)

add_executable(directio src/directio.c)

#Tests
add_executable(simpleboyermoore_test src/simpleboyermoore.h src/simpleboyermoore.c test/simpleboyermoore_test.c)
target_include_directories(simpleboyermoore_test PRIVATE src)

add_executable(
    fix_extract_fix_messages_test
    test/fix_extract_fix_messages_test.c
    test/utils.h
    ${SOURCE} ${HEADERS})
target_include_directories(fix_extract_fix_messages_test PRIVATE src)

add_dependencies(fix_extract_fix_messages_test hdr_histogram)

add_executable(
    spsc_rb_test src/spsc_rb.h src/spsc_rb.c test/spsc_rb_test.c
)
target_include_directories(spsc_rb_test PRIVATE src)

add_executable(
    fixparser_parse_test
    test/fixparser_parse_test.c
    test/utils.h
    ${SOURCE} ${HEADERS}
)
target_include_directories(fixparser_parse_test PRIVATE src)

add_test(fix_exctract_fix_messages fix_extract_fix_messages_test)
add_test(spsc_rb spsc_rb_test)
add_test(simpleboyermoore simpleboyermoore_test)
add_test(fixparser_parse fixparser_parse_test)
