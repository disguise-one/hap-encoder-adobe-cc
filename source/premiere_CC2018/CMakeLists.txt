cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(HapEncoderPlugin)

# this is the one we will install
add_library(HapEncoderPlugin MODULE)

target_sources(HapEncoderPlugin
    PRIVATE
        configure.hpp
        export_settings.cpp
        export_settings.hpp
        main.cpp
        main.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h        
        codec/codec.cpp
        codec/codec.hpp
        codec/texture_converter.cpp
        codec/texture_converter.hpp
        exporter/exporter.cpp
        exporter/exporter.hpp
        movie_writer/movie_writer.cpp
        movie_writer/movie_writer.hpp
)

# make the layout look ok in IDEs
# !!! this isn't working; probably need to get list added above
# source_group(TREE . $<TARGET_PROPERTY:HapEncoderPlugin,SOURCES>)
source_group("Source Files\\codec" REGULAR_EXPRESSION codec/*)
source_group("Source Files\\exporter" REGULAR_EXPRESSION exporter/*)
source_group("Source Files\\movie_writer" REGULAR_EXPRESSION movie_writer/*)

target_link_libraries(HapEncoderPlugin
    PRIVATE
        Hap
        snappy
        squish
        ycocg
        AdobeSdk
        ffmpeg::libavformat
        ffmpeg::libavcodec
        ffmpeg::libavutil
        bcrypt
)

# Include the boost headers
target_include_directories(HapEncoderPlugin
    PRIVATE
        ${ffmpeg_INCLUDE_DIRS}
)

set_target_properties(HapEncoderPlugin
    PROPERTIES
        SUFFIX .prm
)

# hack because squish CMakeLists.txt predates v3.4
# TODO: fix the squish CMakeLists.txt
list(APPEND INCLUDE_DIRS ${squish_SOURCE_DIR})
include_directories(${INCLUDE_DIRS})

install(TARGETS HapEncoderPlugin
  RUNTIME
  LIBRARY DESTINATION .
  COMPONENT plugin
)