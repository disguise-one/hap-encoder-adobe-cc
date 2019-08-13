cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(CodecPluginFoundation)

# this is the one we will install
add_library(CodecPluginFoundation MODULE)

target_sources(CodecPluginFoundation
    PRIVATE
        premiereImporter.cpp
        premiereImporter.hpp
        async_importer.cpp
        async_importer.hpp
        configure.hpp
        export_settings.cpp
        export_settings.hpp
        file_import.rc
        main.cpp
        main.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h
)

target_link_libraries(CodecPluginFoundation
    PRIVATE
        Codec
        CodecRegistration
        CodecFoundationSession
        AdobePremiereSdk
        AdobeShared
)

if (MSVC)
    set_target_properties(CodecPluginFoundation
        PROPERTIES
            SUFFIX ".prm"
    )
endif(MSVC)

if (APPLE)
if (NOT DEFINED Foundation_IDENTIFIER_PREFIX)
message(FATAL_ERROR "Parent must define Foundation_IDENTIFIER_PREFIX")
endif ()
endif (APPLE)

set_target_properties(CodecPluginFoundation
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER ${Foundation_IDENTIFIER_PREFIX}.pr
)

install(
  TARGETS CodecPluginFoundation
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
