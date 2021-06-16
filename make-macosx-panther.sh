#!/bin/bash
CC=gcc-4.0

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory"
	exit 1
fi

# we use the 10.4 SDK at the 10.3 deployment target using Xcode 3.1 on Leopard

unset PPC_SDK
unset PPC_CFLAGS
unset PPC_MACOSX_VERSION_MIN

if [ -d /Developer/SDKs/MacOSX10.4u.sdk ]; then
	PPC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
	PPC_CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -DLEGACY_GL"
	PPC_MACOSX_VERSION_MIN="10.3"
else
	echo "The Mac OS X 10.4 SDK is required to build for Mac OS X Panther"
	exit 1
fi

echo "Building PPC Client/Dedicated Server against \"$PPC_SDK\""
echo

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`

# PPC client and server
if [ -d build/release-darwin-ppc ]; then
	rm -r build/release-darwin-ppc
fi
(ARCH=ppc CC=gcc-4.0 CFLAGS=$PPC_CFLAGS MACOSX_VERSION_MIN=$PPC_MACOSX_VERSION_MIN make -j$NCPU PANTHER=1) || exit 1;

echo

# use the following shell script to build a universal application bundle
export MACOSX_DEPLOYMENT_TARGET="10.3"
export MACOSX_DEPLOYMENT_TARGET_PPC="$PPC_MACOSX_VERSION_MIN"
"./make-macosx-panther-app.sh" release ppc
