cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(AEXPlugin)

# this is the one we will install
add_library(AEXPlugin MODULE)

target_sources(AEXPlugin
    PRIVATE
        output_module.cpp
        output_module.cpp
        output_module.h
        output_module.cpp
        ui.h
        $<$<PLATFORM_ID:Windows>:win/output_module.rc>
        $<$<PLATFORM_ID:Windows>:win/output_module.rc>
        $<$<PLATFORM_ID:Windows>:win/output_module_temp.rc>
        $<$<PLATFORM_ID:Windows>:win/resource.h>
        $<$<PLATFORM_ID:Windows>:win/ui.cpp>
        $<$<PLATFORM_ID:Darwin>:mac/ui.mm>
        $<$<PLATFORM_ID:Darwin>:mac/FNDControlViewController.m>
        $<$<PLATFORM_ID:Darwin>:mac/FNDObjC.h>
        $<$<PLATFORM_ID:Darwin>:mac/FNDControlViewController.h>
        $<$<PLATFORM_ID:Darwin>:mac/PkgInfo>
        $<$<PLATFORM_ID:Darwin>:${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc>
)

if (NOT DEFINED Foundation_IDENTIFIER_PREFIX)
    message(FATAL_ERROR "Parent must define Foundation_IDENTIFIER_PREFIX")
endif ()
if (NOT DEFINED Foundation_CODEC_NAME)
    message(FATAL_ERROR "Parent must define Foundation_CODEC_NAME")
endif ()
if (NOT DEFINED Foundation_OBJC_PREFIX)
string(RANDOM LENGTH 5 ALPHABET ABCDEFGHIJKLMNOPQRSTUVWXYZ
        Foundation_OBJC_PREFIX)
endif ()

set(${PROJECT_NAME}_OSX_BUNDLE_IDENTIFIER ${Foundation_IDENTIFIER_PREFIX}.aex)

if (APPLE)
    set_property(SOURCE mac/ui.mm mac/FNDControlViewController.m
                    APPEND_STRING
                    PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )
    set_property(SOURCE mac/ui.mm mac/FNDControlViewController.m
                    APPEND
                    PROPERTY COMPILE_DEFINITIONS FOUNDATION_OBJC_PREFIX=${Foundation_OBJC_PREFIX} FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER=@\"${${PROJECT_NAME}_OSX_BUNDLE_IDENTIFIER}\"
    )
# Required for AE to load the plugin at all
    set_source_files_properties(mac/PkgInfo
                                PROPERTIES MACOSX_PACKAGE_LOCATION ""
    )
endif (APPLE)

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
        AdobeShared
        Codec
        CodecRegistration
        CodecFoundationSession
        nlohmann_json::nlohmann_json
)

# Include the ffmpeg headers
target_include_directories(AEXPlugin
    PRIVATE
)

if (MSVC)
    set_target_properties(AEXPlugin
        PROPERTIES
            SUFFIX ".aex"
    )
endif(MSVC)

if (APPLE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mac/SettingsViewController.xib.in ${CMAKE_BINARY_DIR}/SettingsViewController.xib)
endif (APPLE)

if (APPLE AND NOT XCODE)
    target_sources(AEXPlugin
        PRIVATE
            ${CMAKE_BINARY_DIR}/SettingsViewController.nib)

    find_program(APPLE_IBTOOL ibtool)
    if (APPLE_IBTOOL)
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/SettingsViewController.nib
                            COMMAND ${APPLE_IBTOOL} --errors --warnings --notices --module "${PROJECT_NAME}" --target-device mac --output-format "human-readable-text" --compile "${CMAKE_BINARY_DIR}/SettingsViewController.nib" "${CMAKE_BINARY_DIR}/SettingsViewController.xib"
                            COMMENT "Creating macOS interface file"
                            DEPENDS ${CMAKE_BINARY_DIR}/SettingsViewController.xib
                            VERBATIM)
    else ()
        message(FATAL_ERROR "Can't find ibtool to build macOS interface file.")
    endif ()
elseif (APPLE)
    target_sources(AEXPlugin
        PRIVATE
            ${CMAKE_BINARY_DIR}/SettingsViewController.xib)
endif ()

if (APPLE)
    find_program(APPLE_RESOURCE Rez)
    if (APPLE_RESOURCE)
        add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
                            COMMAND ${APPLE_RESOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/output_module.r -define __MACH__ -define CODEC_NAME="${Foundation_CODEC_NAME}" -arch x86_64 -useDF -isysroot ${CMAKE_OSX_SYSROOT} -i "$<JOIN:$<TARGET_PROPERTY:AdobeAfterEffectsSdk,INTERFACE_INCLUDE_DIRECTORIES>,;-i;>" -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
                            COMMENT "Creating macOS resource file"
                            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/output_module.r AdobeAfterEffectsSdk
                            VERBATIM
                            COMMAND_EXPAND_LISTS)
    else (APPLE_RESOURCE)
        message(FATAL_ERROR "Can't find Rez to build macOS resource file.")
    endif (APPLE_RESOURCE)
endif (APPLE)

set_target_properties(AEXPlugin
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER ${${PROJECT_NAME}_OSX_BUNDLE_IDENTIFIER}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac/MacOSXBundleInfo.plist.in
)
if (APPLE)
# These could be a RESOURCE property on the target, however CMake gives a misleading warning about RESOURCE properties
    set_source_files_properties(
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rsrc
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    if (XCODE)
        set_source_files_properties(
            ${CMAKE_BINARY_DIR}/SettingsViewController.xib
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    else (XCODE)
        set_source_files_properties(
            ${CMAKE_BINARY_DIR}/SettingsViewController.nib
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endif (XCODE)
endif (APPLE)

install(
  TARGETS AEXPlugin
  RUNTIME
  BUNDLE DESTINATION .
  LIBRARY DESTINATION .
  COMPONENT plugin
)
