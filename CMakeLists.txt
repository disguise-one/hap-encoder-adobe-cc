cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(HapEncoder)

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

set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "notadobe.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2018")
set(CPACK_PACKAGE_VERSION "1.0.6-testi3")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "6")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HapEncoderPlugin")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/license.txt)

set(CPACK_NSIS_PACKAGE_NAME "Hap Encoder Plugin for Adobe CC 2018")

set(CPACK_COMPONENTS_ALL plugin)

# set(CPACK_INSTALL_COMMANDS, "magic command to get plugin directory and copy to it")

# override modules path so we can change the nsis template
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

# This must always be last!
include(CPack)
