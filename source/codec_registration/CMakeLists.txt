cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(CodecRegistration)

# this is the one we will install
add_library(CodecRegistration
    ${CMAKE_CURRENT_LIST_DIR}/util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/util.hpp
)

target_sources(CodecRegistration
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/codec_registration.hpp
)

target_include_directories(CodecRegistration
    PRIVATE
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/
)
