cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(HapEncoderPlugin)

# this is the one we will install
add_library(HapEncoderPlugin MODULE)

target_sources(HapEncoderPlugin
    PRIVATE
        configure.hpp
        hapencode.cpp
        hapencode.hpp
        main.cpp
        main.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h        
        codec/codec.cpp
        codec/codec.hpp
        codec/textureconverter.cpp
        codec/textureconverter.hpp
)

# make the layout look ok in IDEs
# !!! this isn't working; probably need to get list added above
# source_group(TREE . $<TARGET_PROPERTY:HapEncoderPlugin,SOURCES>)
# source_group(TREE codec PREFIX codec)
source_group("Source Files\\codec" REGULAR_EXPRESSION codec/*)

target_link_libraries(HapEncoderPlugin
    PRIVATE
        Hap
        snappy
        joelQT
        squish
        ycocg
        AdobeSdk
        ffmpeg::libavformat
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