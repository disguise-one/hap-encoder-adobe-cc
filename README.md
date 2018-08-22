# Hap Exporter for Adobe CC 2018

This is the community-supplied Hap and Hap Q exporter plugin for Adobe CC 2018.

Development of this plugin was sponsored by disguise, makers of the disguise show production software and hardware

    http://disguise.one

Principal authors of the plugin are

-  Greg Bakker (gbakker@gmail.com)
-  Richard Sykes

Thanks to Tom Butterworth for creating the Hap codec and Vidvox for supporting that development.

Please see license.txt for the licenses of this plugin and the components that were used to create it.

# User Guide

The User Guide can be found [here](doc/user_guide/README.md)

# Development

## Prerequisites

### compiler toolchain

You'll need a compiler environment appropriate to your operating system. The current plugin has been developed on
-  win64 with Microsoft Visual Studio 2017 Professional.
-  macosx with XCode

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
FFmpeg 4.0 is used for output of the .mov format.

It is referenced as a submodule of this repository. Fetch the source for building with

    git submodule init
    git submodule update

The FFMpeg build is not wrapped by the plugin's cmake build process, and must be made in a platform specific process as descibed below.

#### win64
Either install and set environment for your own FFMpeg, or build / install the one in external/ffmpeg as described at

    https://trac.ffmpeg.org/wiki/CompilationGuide/MSVC

For reference, the FFMpeg build for the win64 plugin was created by

-  first installing
 
        http://www.mingw.org/wiki/msys

-  launching a visual studio 2017 developer prompt
-  set visual studio build vars for an x64 build. Something like

        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64

-  running the msys shell from within that prompt

        C:\MinGW\msys\1.0\msys.bat
 
-  going to the external/ffmpeg/FFMPeg directory and then

        ./configure --toolchain=msvc --disable-x86asm --disable-network --disable-everything --enable-muxer=mov --extra-cflags="-MD -arch:AVX"
        make

This will take a while.

#### macosx
Build a local FFmpeg by opening a terminal and moving to external/ffmpeg/FFmpeg. Then

    ./configure --disable-x86asm --disable-network --disable-everything --enable-muxer=mov --disable-zlib --disable-iconv
    make

### Pandoc
Pandoc is required to build the documentation, which is bundled by the installer.

    https://pandoc.org/

#### macos
For macos, the homebrew installation is recommended.

    brew install pandoc

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

### macosx

First create a build directory at the top level, and move into it

    mkdir Release
    cd Release

Invoke cmake to create makefiles etc for a Release build

    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 ..

Then do a build with

    make -j

This will create a plugin at

    Release/source/premiere_CC2018/HapEncoderPlugin.bundle

To create an installer (requires Apple Developer Program membership for signing)

    cd installer
    ./make_mac_installer.sh

TODO: have the installer figure out the right paths to copy plugin+presets to from Adobe's .plist file
