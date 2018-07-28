# Hap Exporter for Adobe CC 2018

This is community-supplied Hap and HapQ exporter plugin for Adobe CC 2018.

Development of this plugin was generously sponsored by disguise, makers of the disguise show production software and hardware
    http://disguise.one

Principal authors of the plugin are
    Greg Bakker (gbakker@gmail.com)
    Richard Sykes

Thanks to the Tom Butterworth for creating the Hap codec and Vidvox for supporting that development.

Please see license.txt for licenses of this plugin and the components that were used to create it.

# Development

## Prerequisites
cmake
    https://cmake.org/install/
    get >= 3.12.0

nsis
    http://nsis.sourceforge.net/Main_Page

##  Building on win64

First create a build directory at the top level, and move into it

    mkdir build
    cd build

Invoke cmake to create a Visual Studio .sln

    cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..

This should create HapEncoder.sln in the current directory. Open it in Visual Studio:

    HapEncoder.sln

The encoder plugin (.prm) is created by building all.
The installer executable is made by building the PACKAGE target, which is excluded from the regular build.