# Hap Exporter for Adobe CC 2018

This is the community-supplied Hap and HapQ exporter plugin for Adobe CC 2018.

Development of this plugin was generously sponsored by disguise, makers of the disguise show production software and hardware
    http://disguise.one

Principal authors of the plugin are
 - Greg Bakker (gbakker@gmail.com)
 - Richard Sykes

Thanks to Tom Butterworth for creating the Hap codec and Vidvox for supporting that development.

Please see license.txt for the licenses of this plugin and the components that were used to create it.

# Development

## Prerequisites

### cmake
cmake creates the build system for the supported target platforms.

    https://cmake.org/install/
    get >= 3.12.0

### NSIS
NSIS is required for win32 installer builds.

    http://nsis.sourceforge.net/Main_Page

### Adobe CC 2018 SDK
Website

    https://www.adobe.io/apis/creativecloud/premierepro.html

Place in

    external/adobe/premiere

### FFMpeg
FFMpeg is included as a submodule to the repository, but its build is not wrapped by the plugin's cmake build process.

The cmake process should be able to locate a prebuilt FFMpeg.

#### win64
Either install and set environment for your own FFMpeg, or build / install the one in external/ffmpeg as described at

    https://trac.ffmpeg.org/wiki/CompilationGuide/MSVC

For reference, the FFMpeg build for the win64 plugin was created by
 - first installing
 
    http://www.mingw.org/wiki/msys

 - launching a visual studio 2017 developer prompt
 - set visual studio build vars for an x64 build. Something like

    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64

 - running the msys shell from within that prompt

    C:\MinGW\msys\1.0\msys.bat
 
 - going to the external/ffmpeg/FFMPeg directory and then

    ./configure --toolchain=msvc --disable-x86asm --disable-network --disable-everything --enable-muxer=mov --extra-cflags="-MD"
    make

This will take a while.

##  Building

### win64

First create a build directory at the top level, and move into it

    mkdir build
    cd build

Invoke cmake to create a Visual Studio .sln

    cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..

This should create HapEncoder.sln in the current directory. Open it in Visual Studio:

    HapEncoder.sln

The encoder plugin (.prm) is created by building all.
The installer executable is made by building the PACKAGE target, which is excluded from the regular build.