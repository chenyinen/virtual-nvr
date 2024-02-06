cmake_minimum_required (VERSION 2.8)

project (decoder)

set(CMAKE_BUILD_TYPE "Debug")

add_library(decoder SHARED)

target_include_directories(decoder PUBLIC ${CMAKE_SOURCE_DIR}/common)
target_include_directories(decoder PUBLIC ${CMAKE_SOURCE_DIR}/common/queue)

target_link_libraries(decoder PUBLIC avcodec avformat avutil swscale pthread)

target_sources(decoder PUBLIC ${CMAKE_SOURCE_DIR}/emu_decoder.c)
target_sources(decoder PUBLIC ${CMAKE_SOURCE_DIR}/common/queue/queue.c ${CMAKE_SOURCE_DIR}/common/queue/queue_internal.c)

add_executable(example)
target_sources(example PUBLIC ${CMAKE_SOURCE_DIR}/example.c)
target_link_libraries(example PUBLIC decoder)
