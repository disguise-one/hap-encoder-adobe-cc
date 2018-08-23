cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

# modules
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/cmake-pandocology")

# project

project(HapEncoder)

set (CMAKE_CXX_STANDARD 17)

if(MSVC)
    # add_compile_options(/arch:AVX2)
    # may want to use this for specific source files
else()
    add_compile_options(-msse4.1)
endif()

# library dependencies
add_subdirectory(external/snappy)
add_subdirectory(external/squish/squish-source)
add_subdirectory(external/hap)
add_subdirectory(external/ffmpeg)
add_subdirectory(external/ycocg)
add_subdirectory(external/adobe)

# documentation
add_subdirectory(doc)

# plugin
add_subdirectory(source/premiere_CC2018)

# installer
# *** this must be last according to CPACK usage directions
add_subdirectory(installer)
