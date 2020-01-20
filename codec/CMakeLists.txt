cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

project(Codec)

# this is the one we will install
add_library(Codec
        codec.cpp
        codec.hpp
        texture_converter.cpp
        texture_converter.hpp
)

target_link_libraries(Codec
    CodecRegistration
    Hap
    snappy
    squish
    ycocg
)

target_include_directories(Codec
    PRIVATE
    INTERFACE
        CodecRegistration
)

# hack because squish CMakeLists.txt predates v3.4
# TODO: fix the squish CMakeLists.txt
list(APPEND INCLUDE_DIRS ${squish_SOURCE_DIR})
include_directories(${INCLUDE_DIRS})