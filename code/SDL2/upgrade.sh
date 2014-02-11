#!/bin/sh

set -e
set -x

# Script to upgrade SDL version to the latest binary release.
# This assumes upstream doesn't change the naming conventions.
# Requires p7zip-full in order to extract DMG and HFS files for OS X.
#
# XXX Currently missing libSDL2main.a for Mac OS X.  The SDL team does not
# provide it in an official release.
VER=2.0.1

rm -f ../libs/win[0-9][0-9]/*SDL* ../libs/macosx/*SDL*
rm -rf include upgrade
mkdir upgrade
cd upgrade
wget http://libsdl.org/release/SDL2-${VER}.dmg \
     http://libsdl.org/release/SDL2-2.0.1-win32-x86.zip \
     http://libsdl.org/release/SDL2-2.0.1-win32-x64.zip \
     http://libsdl.org/release/SDL2-devel-${VER}-mingw.tar.gz \
     http://libsdl.org/release/SDL2-${VER}.tar.gz

# Upgrade the static libraries for Windows
tar -zxvf SDL2-devel-${VER}-mingw.tar.gz \
     SDL2-${VER}/i686-w64-mingw32/lib \
     SDL2-${VER}/x86_64-w64-mingw32/lib
mv SDL2-${VER}/i686-w64-mingw32/lib/*.a   ../../libs/win32/
mv SDL2-${VER}/x86_64-w64-mingw32/lib/*.a ../../libs/win64/
rm ../../libs/win[0-9][0-9]/*_test.a

# Upgrade the dynamic libraries for Windows and OS X
unzip SDL2-2.0.1-win32-x86.zip -d ../../libs/win32/ SDL2.dll
unzip SDL2-2.0.1-win32-x64.zip -d ../../libs/win64/ SDL2.dll
mv ../../libs/win32/SDL2.dll ../../libs/win32/libSDL2.dll
mv ../../libs/win64/SDL2.dll ../../libs/win64/libSDL2.dll
7z x SDL2-${VER}.dmg 2.hfs
7z x 2.hfs SDL2/SDL2.framework/Versions/A/SDL2
mv SDL2/SDL2.framework/Versions/A/SDL2 ../../libs/macosx/libSDL2-${VER}.dylib

# Upgrade the include files (all platforms have the same files)
tar -zxvf SDL2-${VER}.tar.gz SDL2-${VER}/include
mv SDL2-${VER}/include ..
rm ../include/SDL_config.h.in ../include/SDL_config.h.cmake ../include/doxyfile

# Clean up
cd ..
rm -rf upgrade

