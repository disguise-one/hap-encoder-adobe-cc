cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

# win64 installer

set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "HapCommunity")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2018")
set(CPACK_PACKAGE_VERSION "1.0.8")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "8")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HapEncoderPlugin")
set(CPACK_PACKAGE_ICON ${CMAKE_CURRENT_LIST_DIR}/../asset\\\\hap-icon.png)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/../license.txt)

set(CPACK_NSIS_PACKAGE_NAME "Hap Encoder Plugin for Adobe CC 2018")

# set(CPACK_INSTALL_COMMANDS, "magic command to get plugin directory and copy to it")
# currently doing this by overriding template
# TODO: don't override template

# override modules path so we can change the nsis template
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

# macosx installer

# TODO - wrap shell installer here

# generic installer framework
# This must always be last!
include(CPack)
