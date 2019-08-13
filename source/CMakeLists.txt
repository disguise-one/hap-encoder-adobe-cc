cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)

# user-provided codec library
# [this should already have been pulled in by the codec writer]
#   add_subdirectory(codec_registration)

# import / export input / output sessions
add_subdirectory(session)

# plugins
add_subdirectory(plugin)
