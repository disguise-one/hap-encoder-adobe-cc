cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(AdobeSdk LANGUAGES CXX)

add_library(AdobeSdk INTERFACE)
target_sources(AdobeSdk
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
target_include_directories(AdobeSdk INTERFACE premiere/Headers premiere/Utils/)
