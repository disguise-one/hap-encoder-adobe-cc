cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(NotchLCAEXPlugin)

# this is the one we will install
add_library(NotchLCAEXPlugin MODULE)

target_sources(NotchLCAEXPlugin
    PRIVATE
        after_effects/output_module.cpp
        after_effects/output_module.h
        after_effects/output_module.cpp
        after_effects/ui.h
        after_effects/win/output_module.rc
        after_effects/win/output_module_temp.rc
        after_effects/win/resource.h
        after_effects/win/ui.cpp
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
)

# # make the layout look ok in IDEs
# source_group("Source Files\\exporter" REGULAR_EXPRESSION premiere_CC2018/exporter/*)
# source_group("Source Files\\movie_writer" REGULAR_EXPRESSION premiere_CC2018/movie_writer/*)

find_library(CoreFoundation CoreFoundation)
find_library(CoreVideo CoreVideo)
find_library(Accelerate Accelerate)

if (MSVC)
else (MSVC)
   find_package(PkgConfig REQUIRED)
   pkg_check_modules (GLIB2   glib-2.0)
   include(findZLIB)
endif(MSVC)

target_link_libraries(NotchLCAEXPlugin
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
        $<$<PLATFORM_ID:Darwin>:${CoreFoundation}>
        $<$<PLATFORM_ID:Darwin>:${CoreVideo}>
        $<$<PLATFORM_ID:Darwin>:${Accelerate}>
        $<$<PLATFORM_ID:Darwin>:${GLIB2}>
        $<$<PLATFORM_ID:Darwin>:${ZLIB}>
)

# Include the ffmpeg headers
target_include_directories(NotchLCAEXPlugin
    PRIVATE
        ${ffmpeg_INCLUDE_DIRS}
)

if (MSVC)
    set_target_properties(NotchLCAEXPlugin
        PROPERTIES
            SUFFIX ".aex"
    )
endif(MSVC)

set_target_properties(NotchLCAEXPlugin
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER org.hapcommunity.HapCodecPlugin
)

install(
  TARGETS NotchLCAEXPlugin
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
