cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(ycocg)

add_library(ycocg STATIC)

target_sources(ycocg
    PRIVATE
        YCoCg.c
        YCoCgDXT.cpp
        YCoCgDXT.h
        ImageMath.c
        ImageMath.h
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/YCoCg.h
        ${CMAKE_CURRENT_LIST_DIR}/YCoCgDXT.h
)

target_include_directories(ycocg
    PUBLIC
        .
)
