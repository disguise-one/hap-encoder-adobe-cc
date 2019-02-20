cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(CodecPluginFoundation)

# this is the one we will install
add_library(CodecPluginFoundation MODULE)

target_sources(CodecPluginFoundation
    PRIVATE
        async_importer.cpp
        async_importer.hpp
        configure.hpp
        export_settings.cpp
        export_settings.hpp
        file_import.rc
        freelist.hpp
        main.cpp
        main.hpp
        importer.cpp
        importer.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h
        # codec/texture_converter.cpp
        # codec/texture_converter.hpp
        exporter/exporter.cpp
        exporter/exporter.hpp
        movie/ffmpeg_helpers.hpp
        movie/movie_reader.cpp
        movie/movie_reader.hpp
        movie_writer/movie_writer.cpp
        movie_writer/movie_writer.hpp
)

# make the layout look ok in IDEs
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

target_link_libraries(CodecPluginFoundation
    PRIVATE
        CodecRegistration
        Codec
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
target_include_directories(CodecPluginFoundation
    PRIVATE
        ${ffmpeg_INCLUDE_DIRS}
)

if (MSVC)
    set_target_properties(CodecPluginFoundation
        PROPERTIES
            SUFFIX ".prm"
    )
endif(MSVC)

set_target_properties(CodecPluginFoundation
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER org.hapcommunity.HapCodecPlugin
)

install(
  TARGETS CodecPluginFoundation
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
