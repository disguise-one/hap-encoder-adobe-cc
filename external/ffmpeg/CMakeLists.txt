cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(ffmpeg)

# could put a build step here
# ExternalProject_Add(
#    PREFIX
#        FFmpeg
#
#)

add_library(libavcodec STATIC IMPORTED GLOBAL)
add_library(libavformat STATIC IMPORTED GLOBAL)
add_library(libavutil STATIC IMPORTED GLOBAL)

set_target_properties(libavcodec
    PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/FFmpeg/libavcodec/libavcodec.a
)
set_target_properties(libavformat
    PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/FFmpeg/libavformat/libavformat.a
)
set_target_properties(libavutil
    PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/FFmpeg/libavutil/libavutil.a
)

target_include_directories(libavcodec INTERFACE
    FFmpeg
)
target_include_directories(libavformat INTERFACE
    FFmpeg
)
target_include_directories(libavutil INTERFACE
    FFmpeg
)

add_library(ffmpeg::libavcodec ALIAS libavcodec)
add_library(ffmpeg::libavformat ALIAS libavformat)
add_library(ffmpeg::libavutil ALIAS libavutil)

# add_include(ffmpeg ${CMAKE_CURRENT_LIST_DIR}/FFmpeg)
