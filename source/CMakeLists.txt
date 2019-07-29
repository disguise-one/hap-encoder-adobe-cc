cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(AEXPlugin)

# this is the one we will install
add_library(AEXPlugin MODULE)

target_sources(AEXPlugin
    PRIVATE
        after_effects/output_module.cpp
        after_effects/output_module.h
        after_effects/output_module.cpp
        after_effects/ui.h
        premiere_CC2018/configure.hpp
        premiere_CC2018/freelist.hpp
        premiere_CC2018/string_conversion.cpp
        premiere_CC2018/string_conversion.hpp
        premiere_CC2018/targetver.h
        premiere_CC2018/exporter/exporter.cpp
        premiere_CC2018/exporter/exporter.hpp
        premiere_CC2018/movie/ffmpeg_helpers.hpp
        premiere_CC2018/movie_writer/movie_writer.cpp
        premiere_CC2018/movie_writer/movie_writer.hpp
        $<$<PLATFORM_ID:Windows>:after_effects/win/output_module.rc>
        $<$<PLATFORM_ID:Windows>:after_effects/win/output_module.rc>
        $<$<PLATFORM_ID:Windows>:after_effects/win/output_module_temp.rc>
        $<$<PLATFORM_ID:Windows>:after_effects/win/resource.h>
        $<$<PLATFORM_ID:Windows>:after_effects/win/ui.cpp>
        $<$<PLATFORM_ID:Darwin>:after_effects/mac/ui.mm>
        $<$<PLATFORM_ID:Darwin>:after_effects/mac/PkgInfo>
        $<$<PLATFORM_ID:Darwin>:${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc>
)

if (APPLE)
# Required for AE to load the plugin at all
    set_source_files_properties(after_effects/mac/PkgInfo
                                PROPERTIES MACOSX_PACKAGE_LOCATION "")
endif (APPLE)

# # make the layout look ok in IDEs
# source_group("Source Files\\exporter" REGULAR_EXPRESSION premiere_CC2018/exporter/*)
# source_group("Source Files\\movie_writer" REGULAR_EXPRESSION premiere_CC2018/movie_writer/*)

find_library(AppKit AppKit)
find_library(CoreFoundation CoreFoundation)
find_library(CoreVideo CoreVideo)
find_library(Accelerate Accelerate)

if (MSVC)
else (MSVC)
   find_package(PkgConfig REQUIRED)
   pkg_check_modules (GLIB2   glib-2.0)
   include(findZLIB)
endif(MSVC)

target_link_libraries(AEXPlugin
    PRIVATE
        AdobeAfterEffectsSdk
        CodecRegistration
        Codec
        ffmpeg::libavformat
        ffmpeg::libavcodec
        ffmpeg::libavutil
        nlohmann_json::nlohmann_json
	# need this for ffmpeg on windows; TODO: get rid of it
        $<$<PLATFORM_ID:Windows>:bcrypt>
        $<$<PLATFORM_ID:Darwin>:${AppKit}>
        $<$<PLATFORM_ID:Darwin>:${CoreFoundation}>
        $<$<PLATFORM_ID:Darwin>:${CoreVideo}>
        $<$<PLATFORM_ID:Darwin>:${Accelerate}>
        $<$<PLATFORM_ID:Darwin>:${GLIB2}>
        $<$<PLATFORM_ID:Darwin>:${ZLIB}>
)

# Include the ffmpeg headers
target_include_directories(AEXPlugin
    PRIVATE
        ${ffmpeg_INCLUDE_DIRS}
)

if (MSVC)
    set_target_properties(AEXPlugin
        PROPERTIES
            SUFFIX ".aex"
    )
endif(MSVC)

# TODO: get CODEC_NAME from outside foundation
if (APPLE)
    find_program(APPLE_RESOURCE Rez)
    if (APPLE_RESOURCE)
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
                            COMMAND ${APPLE_RESOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/after_effects/output_module.r -define __MACH__ -define CODEC_NAME="Quicktime HAP Format" -arch x86_64 -useDF -isysroot ${CMAKE_OSX_SYSROOT} -i "$<JOIN:$<TARGET_PROPERTY:AdobeAfterEffectsSdk,INTERFACE_INCLUDE_DIRECTORIES>,;-i;>" -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
                            COMMENT "Creating macOS resource file"
                            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/after_effects/output_module.r AdobeAfterEffectsSdk
                            VERBATIM
                            COMMAND_EXPAND_LISTS)
    else (APPLE_RESOURCE)
        message(FATAL_ERROR "Can't find Rez to build macOS resource file.")
    endif (APPLE_RESOURCE)
endif (APPLE)

set_target_properties(AEXPlugin
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER org.hapcommunity.HapCodecPlugin
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/after_effects/mac/MacOSXBundleInfo.plist.in
)
if (APPLE)
# This could be a RESOURCE property on the target, however CMake gives a misleading warning about RESOURCE properties
    set_source_files_properties(
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif (APPLE)

install(
  TARGETS AEXPlugin
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
