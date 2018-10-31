cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

# presets asset files

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../asset/encoder_preset/Hap\ Alpha-Only.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/../asset/encoder_preset/Hap\ Alpha.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/../asset/encoder_preset/Hap\ Q\ Alpha.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/../asset/encoder_preset/Hap\ Q.epr
        ${CMAKE_CURRENT_SOURCE_DIR}/../asset/encoder_preset/Hap.epr
    DESTINATION
        "Presets"
    COMPONENT
        presets
)

# win64 installer

#   as of publication, details of where to place installed files comes
#   from emailed advice; SDK docs are for previous versions.
#
#   Win64
#       Nov 17 CC SDK reads:
#
#       look at registry 
#           Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Adobe\Premiere Pro\12.0
#           Key:CommonExporterPresetsPath
#       which will say
#           [User folder]\AppData\Roaming\Adobe\Common\AME\[version]\Presets\
#
#       but this is out of date, and current advice is to put them in
#           C:\Users\[USERID]\Documents\Adobe\Adobe Media Encoder\[MAJOR_VERSION.0]\Preseets
#
#   TODO: update to use registry keys once this is possible
#

set(CPACK_PACKAGE_NAME "HapEncoder")
set(CPACK_PACKAGE_VENDOR "HapCommunity")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HapEncoder - Hap encoding plugin for Adobe CC 2018")
set(CPACK_PACKAGE_VERSION "1.0.9-rc1")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "9")
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

# NSIS component specific settings
set(CPACK_NSIS_plugin_INSTALL_DIRECTORY "$PROGRAMFILES64\\\\Adobe\\\\Common\\\\Plug-ins\\\\7.0\\\\MediaCore\\\\")
set(CPACK_NSIS_presets_INSTALL_DIRECTORY "$PROFILE\\\\Documents\\\\Adobe\\\\Adobe\ Media\ Encoder\\\\12.0\\\\")


# set(CPACK_INSTALL_COMMANDS, "magic command to get plugin directory and copy to it")
# currently doing this by overriding template
# TODO: don't override template

# override modules path so we can change the nsis template
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

# macosx installer

# TODO - wrap shell installer here

# generic installer framework
# This must always be after CPACK settings
include(CPack)

# components to include to installer
cpack_add_component(plugin
    DISPLAY_NAME "Plugin"
    DESCRIPTION "plugin module"
    REQUIRED
)
cpack_add_component(presets
    DISPLAY_NAME "Presets"
    DESCRIPTION "presets"
    OPTIONAL
)
cpack_add_component(user_guide
    DISPLAY_NAME "User Guide"
    DESCRIPTION "user guide"
    OPTIONAL
)

