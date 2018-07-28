cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(Hap)

add_library(Hap STATIC)

target_sources(Hap
    PRIVATE
        hap.c
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hap.h
)

target_link_libraries(Hap
    PUBLIC
        snappy
)

target_include_directories(Hap
    PUBLIC
        .
)