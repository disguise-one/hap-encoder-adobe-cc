cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

# presets asset files

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/Hap\ Alpha-Only.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/Hap\ Alpha.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/Hap\ Q\ Alpha.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/Hap\ Q.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/encoder_preset/Hap.epr
    DESTINATION
        "Presets"
    COMPONENT
        presets
)

set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "HapCommunity")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2018")
set(CPACK_PACKAGE_VERSION "1.1.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HapEncoderPlugin")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_LIST_DIR}/../license.txt)

set(CPACK_COMPONENTS_ALL presets user_guide plugin)

# NSIS specific settings
set(CPACK_NSIS_URL_INFO_ABOUT "https://hap.video")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_PACKAGE_NAME "Hap Encoder Plugin for Adobe CC 2018")
set(CPACK_NSIS_MENU_LINKS "doc\\\\HapExporterPlugin_for_AdobeCC2018.html;User Guide")
set(CPACK_NSIS_HELP_LINK "https://github.com/GregBakker/hap-adobe-premiere-plugin")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/GregBakker/hap-adobe-premiere-plugin/tree/master/doc/user_guide/")
set(CPACK_NSIS_CONTACT "happlugin@disguise.one")

# set(bitmap_path ${CMAKE_CURRENT_LIST_DIR}/../asset/install_image.bmp)
# STRING(REPLACE "/" "\\" bitmap_path  ${bitmap_path}) 
# set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${bitmap_path})
