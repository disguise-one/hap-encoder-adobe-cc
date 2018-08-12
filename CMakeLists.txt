cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(HapEncoder)

set (CMAKE_CXX_STANDARD 17)

if(MSVC)
    # add_compile_options(/arch:AVX2)
    # may want to use this for specific source files
else()
    add_compile_options(-msse4.1)
endif()

add_subdirectory(external/snappy)
add_subdirectory(external/squish/squish-source)
add_subdirectory(external/hap)
add_subdirectory(external/ffmpeg)
add_subdirectory(external/ycocg)
add_subdirectory(external/adobe)

add_subdirectory(source/premiere_CC2018)

# win64
set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "HapCommunity")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2018")
set(CPACK_PACKAGE_VERSION "1.0.7")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "7")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HapEncoderPlugin")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/license.txt)

set(CPACK_NSIS_PACKAGE_NAME "Hap Encoder Plugin for Adobe CC 2018")

set(CPACK_COMPONENTS_ALL plugin)

# set(CPACK_INSTALL_COMMANDS, "magic command to get plugin directory and copy to it")

# override modules path so we can change the nsis template
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})


# macosx
# !!! set(CPACK_DMG_VOLUME_NAME "HapEncoder")
# !!! set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK ON)

set(CPACK_BUNDLE_NAME "HapEncoder")
set(CPACK_BUNDLE_PLIST "installer/Info.plist"???)
set(CPACK_BUNDLE_ICON ???)
# set(CPACK_BUNDLE_STARTUP_COMMAND Path to a startup script. This is a path to an executable or script that will be run whenever an end-user double-clicks the generated bundle in the OSX Finder. Optional.
# CPACK_BUNDLE_APPLE_CERT_APP The name of your Apple supplied code signing certificate for the application. The name usually takes the form “Developer ID Application: [Name]” or “3rd Party Mac Developer Application: [Name]”. If this variable is not set the application will not be signed.



# This must always be last!
include(CPack)
