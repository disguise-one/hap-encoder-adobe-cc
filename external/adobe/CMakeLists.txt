cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(AdobeSdk LANGUAGES CXX)

add_library(AdobePremiereSdk INTERFACE)
target_sources(AdobePremiereSdk
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKEntry.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKExport.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKExportFileSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKExporterUtilitySuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKImport.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKMALErrors.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKMarkerSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKMediaAcceleratorSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKMemoryManagerSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKPPixSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKRollCrawlSuite.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/PrSDKTypes.h
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Headers/SPBasic.h
)
target_include_directories(AdobePremiereSdk INTERFACE premiere/Headers premiere/Utils/)


add_library(AdobeAfterEffectsSdk INTERFACE)
target_sources(AdobeAfterEffectsSdk
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/adobesdk/config/PreConfig.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/adobesdk/config/PostConfig.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/adobesdk/drawbotsuite/DrawBotSuiteTypes.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AEConfig.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AEConfig.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_GeneralPlug.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_GeneralPlugPre.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_GeneralPlugPost.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/A.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_AdvEffectSuites.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_Effect.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectCB.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectCBSuites.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectPixelFormat.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectSuites.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectSuitesHelper.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_EffectUI.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_GeneralPlug.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_IO.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_IO_FileExt.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/AE_Macros.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/FIEL_Public.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/PT_Public.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPAccess.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPAdapts.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPBasic.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPCaches.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPConfig.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPErrorCodes.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPFiles.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPMData.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPPiPL.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPPlugs.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPProps.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPStrngs.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPSuites.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Headers/SP/SPTypes.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Util/AEGP_SuiteHandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Util/AEGP_SuiteHandler.h
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Util/MissingSuiteError.cpp
        ${CMAKE_CURRENT_LIST_DIR}/AfterEffectsSDK/Examples/Util/entry.h
)
target_include_directories(AdobeAfterEffectsSdk INTERFACE AfterEffectsSDK/Examples/Headers AfterEffectsSDK/Examples/Headers/SP AfterEffectsSDK/Examples/Util)
