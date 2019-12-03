cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "HapCommunity")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2019")
set(CPACK_PACKAGE_VERSION "1.1.0-prealpha1")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0-alpha1")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HapEncoderPlugin")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/../license.txt)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_LIST_DIR}/ReadMe.txt)

if (APPLE)
    set(CPACK_PREFLIGHT_PLUGIN_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/preinstall")
endif (APPLE)

# NSIS specific settings
set(CPACK_NSIS_URL_INFO_ABOUT "https://hap.video")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_PACKAGE_NAME "Hap Encoder Plugin for Adobe CC 2019")
set(CPACK_NSIS_HELP_LINK "https://github.com/disguise-one/hap-encoder-adobe-cc")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/disguise-one/hap-encoder-adobe-cc/tree/master/doc/user_guide/")
set(CPACK_NSIS_CONTACT "happlugin@disguise.one")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
  "Delete \\\"$PROGRAMFILES64\\\\Adobe\\\\Common\\\\Plug-ins\\\\7.0\\\\MediaCore\\\\CodecPluginFoundation.prm\\\""
  "\\nDelete \\\"$PROGRAMFILES64\\\\Adobe\\\\Common\\\\Plug-ins\\\\7.0\\\\MediaCore\\\\HapEncoderPlugin.prm\\\""
)

# set(bitmap_path ${CMAKE_CURRENT_LIST_DIR}/../asset/install_image.bmp)
# STRING(REPLACE "/" "\\" bitmap_path  ${bitmap_path}) 
# set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${bitmap_path})
