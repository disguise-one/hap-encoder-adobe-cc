cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(AdobeShared)

set (CMAKE_CXX_STANDARD 17)

# this is the one we will install
add_library(AdobeShared
    string_conversion.cpp
)

target_sources(AdobeShared
    PUBLIC
        string_conversion.hpp
)

target_include_directories(AdobeShared
    PRIVATE
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)
