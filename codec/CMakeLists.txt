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
)

target_include_directories(Codec
    PRIVATE
    INTERFACE
        CodecRegistration
)
