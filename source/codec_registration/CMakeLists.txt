cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(CodecRegistration)

# this is the one we will install
add_library(CodecRegistration INTERFACE)

target_sources(CodecRegistration
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/codec_registration.hpp
)

target_include_directories(CodecRegistration
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/
)
