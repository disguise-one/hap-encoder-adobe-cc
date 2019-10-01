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

# !!! TODO: Determine convention for naming variables required by foundation
set(Foundation_IDENTIFIER_PREFIX org.hapcommunity.HapCodecPlugin)
set(Foundation_CODEC_NAME HAP)
set(Foundation_PRESETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/")

# codec declarations that need to be compiled in
# !!! TODO: make these into variables that are used in build steps
add_definitions(-DFOUNDATION_FILE_IMPORT_FOUR_CC=0x48415000L)  # hex for HAP\0, needed as unique importer id so After Effects picks it up as an importer
add_definitions(-DCODEC_NAME="${Foundation_CODEC_NAME}")
add_definitions(-DCODEC_NAME_WITH_HEX_SIZE_PREFIX="\\x07HAP\\x0\\x0\\x0\\x0")   # !!! hack - size must be 0x7 atm, and padded to 7-bytes- we're not using the recommended resource builder step that compiles the correct sizes around this

# foundation
add_subdirectory(external/foundation)

# installer
include(installer/CodecInstaller.cmake)

# foundation installer framework
# *** this must be last according to CPACK usage directions
add_subdirectory(external/foundation/installer)

# codec-specific components
# must be added *after* CPACK usage in external/foundation/installer above
