cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(CodecFoundationSession)

# this is the one we will install
add_library(CodecFoundationSession)

target_sources(CodecFoundationSession
    PUBLIC
        exporter.hpp
        freelist.hpp
        ffmpeg_helpers.hpp
        importer.hpp
        movie_reader.hpp
        movie_writer.hpp
        sample_cache.hpp
    PRIVATE
        exporter.cpp
        importer.cpp
        movie_reader.cpp
        movie_writer.cpp
)

find_library(CoreFoundation CoreFoundation)
find_library(CoreVideo CoreVideo)
find_library(Accelerate Accelerate)

if (MSVC)
else (MSVC)
   find_package(PkgConfig REQUIRED)
   pkg_check_modules (GLIB2   glib-2.0)
   include(findZLIB)
endif(MSVC)

target_link_libraries(CodecFoundationSession
    PUBLIC
        ffmpeg::libavformat
        ffmpeg::libavcodec
        ffmpeg::libavutil        
    PRIVATE
        CodecRegistration
        Codec
	# need this for ffmpeg on windows; TODO: get rid of it
        $<$<PLATFORM_ID:Windows>:bcrypt>
        $<$<PLATFORM_ID:Darwin>:${CoreFoundation}>
        $<$<PLATFORM_ID:Darwin>:${CoreVideo}>
        $<$<PLATFORM_ID:Darwin>:${Accelerate}>
        $<$<PLATFORM_ID:Darwin>:${GLIB2}>
        $<$<PLATFORM_ID:Darwin>:${ZLIB}>
)

# Include the ffmpeg headers
target_include_directories(CodecFoundationSession
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${ffmpeg_INCLUDE_DIRS}
    PRIVATE
)
