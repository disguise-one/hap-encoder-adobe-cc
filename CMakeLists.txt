cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(CodecPlugin)

# foundation variables
# ------------------------

set(Foundation_IDENTIFIER_PREFIX org.hapcommunity.HapCodecPlugin)
set(Foundation_CODEC_NAME HAP)
set(Foundation_PRESETS
    "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/Hap.epr"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/Hap Alpha.epr"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/Hap Alpha-Only.epr"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/Hap Q.epr"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/ame-12.0/Hap Q Alpha.epr"
)
set(Foundation_FILE_IMPORT_FOUR_CC=0x48415000L)    # hex for HAP\0, needed as unique importer id so After Effects picks it up as an importer
set(Foundation_CODEC_NAME_WITH_HEX_SIZE_PREFIX="\\x07HAP\\x0\\x0\\x0\\x0")    # !!! hack - size must be 0x7 atm, and padded to 7-bytes- we're not using the recommended resource builder step that compiles the correct sizes around this

# project-wide build settings
# ----------------------------

# all platforms: group targets in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# macosx: release on Mojave and Catalina
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")
endif(APPLE)

if(MSVC)
    # add_compile_options(/arch:AVX2)
    # may want to use this for specific source files
else()
    add_compile_options(-msse4.1)
endif()

set (CMAKE_CXX_STANDARD 17)

# foundation interface to be populated
# ------------------------

# - externals needed by codec registration interface
add_subdirectory(external/foundation/external)
# - defines codec registration interface
add_subdirectory(external/foundation/source/codec_registration)


# codec-specific external libs
add_subdirectory(external/snappy)
add_subdirectory(external/squish/squish-source)
add_subdirectory(external/hap)
add_subdirectory(external/ycocg)

# codec implementation in its own library
# ------------------------
add_subdirectory(codec)

# foundation plugins
# ------------------------
add_subdirectory(external/foundation)

# codec installer information for foundation
# ------------------------
include(installer/CodecInstaller.cmake)

# foundation installer framework
# *** this must be last according to CPACK usage directions
add_subdirectory(external/foundation/installer)

# codec-specific components
# must be added *after* CPACK usage in external/foundation/installer above
