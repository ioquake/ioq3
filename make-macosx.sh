#!/bin/bash
#

# Let's make the user give us a target build system

if [ $# -ne 1 ]; then
	echo "Usage:   $0 target_architecture"
	echo "Example: $0 x86"
	echo "other valid options are arm64, x86_64 or ppc"
	echo
	echo "If you don't know or care about architectures please consider using make-macosx-ub.sh instead of this script."
	exit 1
fi

if [ "$1" == "x86" ]; then
	BUILDARCH=x86
elif [ "$1" == "x86_64" ]; then
	BUILDARCH=x86_64
elif [ "$1" == "ppc" ]; then
	BUILDARCH=ppc
elif [ "$1" == "arm64" ]; then
	BUILDARCH=arm64
else
	echo "Invalid architecture: $1"
	echo "Valid architectures are arm64, x86_64, x86, or ppc"
	exit 1
fi

CC=gcc-4.0
DESTDIR=build/release-darwin-${BUILDARCH}

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the ioquake3 build directory"
	exit 1
fi

# we want to use the oldest available SDK for max compatibility. However 10.4 and older
# can not build 64bit binaries, making 10.5 the minimum version.   This has been tested 
# with xcode 3.1 (xcode31_2199_developerdvd.dmg).  It contains the 10.5 SDK and a decent
# enough gcc to actually compile ioquake3
# For PPC macs, G4's or better are required to run ioquake3.

unset ARCH_SDK
unset ARCH_CFLAGS
unset ARCH_MACOSX_VERSION_MIN

# SDL 2.0.1 (ppc) supports MacOSX 10.5
# SDL 2.0.5+ (x86, x86_64) supports MacOSX 10.6 and later
if [ $BUILDARCH = "ppc" ]; then
	if [ -d /Developer/SDKs/MacOSX10.5.sdk ]; then
		ARCH_SDK=/Developer/SDKs/MacOSX10.5.sdk
		ARCH_CFLAGS="-isysroot /Developer/SDKs/MacOSX10.5.sdk"
	fi
	ARCH_MACOSX_VERSION_MIN="10.5"
elif [ $BUILDARCH = "x86" ]; then
	if [ -d /Developer/SDKs/MacOSX10.6.sdk ]; then
		ARCH_SDK=/Developer/SDKs/MacOSX10.6.sdk
		ARCH_CFLAGS="-isysroot /Developer/SDKs/MacOSX10.6.sdk"
	fi
	ARCH_MACOSX_VERSION_MIN="10.6"
elif [ $BUILDARCH = "x86_64" ]; then
	if [ -d /Developer/SDKs/MacOSX10.6.sdk ]; then
		ARCH_SDK=/Developer/SDKs/MacOSX10.6.sdk
		ARCH_CFLAGS="-isysroot /Developer/SDKs/MacOSX10.6.sdk"
		ARCH_MACOSX_VERSION_MIN="10.6"
	else
		if [ -n "$MACOSX_VERSION_MIN" ]; then
			DEFAULT_SDK=$MACOSX_VERSION_MIN
		else
			# trying to find default SDK version is hard
			# macOS 10.15 requires -sdk macosx but 10.11 doesn't support it
			# macOS 10.6 doesn't have -show-sdk-version
			DEFAULT_SDK=`xcrun -sdk macosx -show-sdk-version 2> /dev/null`
			if [ -z "$DEFAULT_SDK" ]; then
				DEFAULT_SDK=`xcrun -show-sdk-version 2> /dev/null`
			fi
			if [ -z "$DEFAULT_SDK" ]; then
				echo "Error: Unable to determine macOS SDK version."
				echo "On macOS 10.6 to 10.8 run:"
				echo "  MACOSX_VERSION_MIN=10.6 $0 $BUILDARCH"
				echo "On macOS 10.9 or later run:"
				echo "  MACOSX_VERSION_MIN=10.9 $0 $BUILDARCH"
				exit 1
			fi
		fi

		if [ "$DEFAULT_SDK" = "10.6" ] \
		  || [ "$DEFAULT_SDK" = "10.7" ] \
		  || [ "$DEFAULT_SDK" = "10.8" ]; then
			ARCH_MACOSX_VERSION_MIN="10.6"
		else
			ARCH_MACOSX_VERSION_MIN="10.9"
		fi
	fi
elif [ $BUILDARCH = "arm64" ]; then
	ARCH_MACOSX_VERSION_MIN="11.0"
fi


echo "Building ${BUILDARCH} Client/Dedicated Server against \"$ARCH_SDK\""
sleep 3

if [ ! -d $DESTDIR ]; then
	mkdir -p $DESTDIR
fi

# For parallel make on multicore boxes...
SYSCTL_PATH=`command -v sysctl 2> /dev/null`
if [ -n "$SYSCTL_PATH" ]; then
	NCPU=`sysctl -n hw.ncpu`
else
	# osxcross on linux
	NCPU=`nproc`
fi

# intel client and server
#if [ -d build/release-darwin-${BUILDARCH} ]; then
#	rm -r build/release-darwin-${BUILDARCH}
#fi
(PLATFORM=darwin ARCH=${BUILDARCH} CFLAGS=$ARCH_CFLAGS MACOSX_VERSION_MIN=$ARCH_MACOSX_VERSION_MIN make -j$NCPU) || exit 1;

# use the following shell script to build an application bundle
export MACOSX_DEPLOYMENT_TARGET="${ARCH_MACOSX_VERSION_MIN}"
export MACOSX_DEPLOYMENT_TARGET_PPC=
export MACOSX_DEPLOYMENT_TARGET_X86=
export MACOSX_DEPLOYMENT_TARGET_X86_64=
export MACOSX_DEPLOYMENT_TARGET_ARM64=
"./make-macosx-app.sh" release ${BUILDARCH}
