cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(HapEncoderPlugin)

# this is the one we will install
add_library(HapEncoderPlugin MODULE)

target_sources(HapEncoderPlugin
    PRIVATE
        configure.hpp
        export_settings.cpp
        export_settings.hpp
        main.cpp
        main.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h        
        codec/codec.cpp
        codec/codec.hpp
        codec/texture_converter.cpp
        codec/texture_converter.hpp
        exporter/exporter.cpp
        exporter/exporter.hpp
        movie_writer/movie_writer.cpp
        movie_writer/movie_writer.hpp
)

# make the layout look ok in IDEs
# !!! this isn't working; probably need to get list added above
# source_group(TREE . $<TARGET_PROPERTY:HapEncoderPlugin,SOURCES>)
source_group("Source Files\\codec" REGULAR_EXPRESSION codec/*)
source_group("Source Files\\exporter" REGULAR_EXPRESSION exporter/*)
source_group("Source Files\\movie_writer" REGULAR_EXPRESSION movie_writer/*)

find_library(CoreFoundation CoreFoundation)
find_library(CoreVideo CoreVideo)
find_library(Accelerate Accelerate)

if (MSVC)
else (MSVC)
   find_package(PkgConfig REQUIRED)
   pkg_check_modules (GLIB2   glib-2.0)
   include(findZLIB)
endif(MSVC)

target_link_libraries(HapEncoderPlugin
    PRIVATE
        Hap
        snappy
        squish
        ycocg
        AdobeSdk
        ffmpeg::libavformat
        ffmpeg::libavcodec
        ffmpeg::libavutil
	# need this for ffmpeg on windows; TODO: get rid of it
        $<$<PLATFORM_ID:Windows>:bcrypt>
        $<$<PLATFORM_ID:Darwin>:${CoreFoundation}>
        $<$<PLATFORM_ID:Darwin>:${CoreVideo}>
        $<$<PLATFORM_ID:Darwin>:${Accelerate}>
        $<$<PLATFORM_ID:Darwin>:${GLIB2}>
        $<$<PLATFORM_ID:Darwin>:${ZLIB}>
)

# Include the ffmpeg headers
target_include_directories(HapEncoderPlugin
    PRIVATE
        ${ffmpeg_INCLUDE_DIRS}
)

set_target_properties(HapEncoderPlugin
    PROPERTIES
        $<$<PLATFORM_ID:Windows>:SUFFIX .prm>
	BUNDLE TRUE
	MACOSX_BUNDLE_GUI_IDENTIFIER org.hapcommunity.HapEncoderPlugin
)

# hack because squish CMakeLists.txt predates v3.4
# TODO: fix the squish CMakeLists.txt
list(APPEND INCLUDE_DIRS ${squish_SOURCE_DIR})
include_directories(${INCLUDE_DIRS})

install(
  TARGETS HapEncoderPlugin
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
