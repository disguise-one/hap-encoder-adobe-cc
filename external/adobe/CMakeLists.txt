cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(AdobeSdk)

add_library(AdobeSdk STATIC)
target_sources(AdobeSdk
    PRIVATE
        premiere/Utils/SDK_Segment_Utils.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/premiere/Utils/SDK_Segment_Utils.h
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
target_include_directories(AdobeSdk PUBLIC premiere/Headers premiere/Utils/)
