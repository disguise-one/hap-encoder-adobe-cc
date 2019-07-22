cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

# modules
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/cmake-pandocology")

# foundation includes - defines codec registration interface
add_subdirectory(external/foundation/source/codec_registration)

# project

project(CodecPlugin)

set (CMAKE_CXX_STANDARD 17)

if(MSVC)
    # add_compile_options(/arch:AVX2)
    # may want to use this for specific source files
else()
    add_compile_options(-msse4.1)
endif()

# codec-specific external libs
add_subdirectory(external/snappy)
add_subdirectory(external/squish/squish-source)
add_subdirectory(external/hap)
add_subdirectory(external/ycocg)

# codec in its own library
add_subdirectory(codec)

# documentation
add_subdirectory(doc)

# foundation
add_definitions(-DFOUNDATION_FILE_IMPORT_FOUR_CC=0x48415000L)  # hex for HAP\0, needed as unique importer id so After Effects picks it up as an importer
add_subdirectory(external/foundation)

# # documentation
# add_subdirectory(doc)

# # plugin
# add_subdirectory(source/premiere_CC2018)

# installer
include(installer/CodecInstaller.cmake)

# foundation installer framework
# *** this must be last according to CPACK usage directions
add_subdirectory(external/foundation/installer)
