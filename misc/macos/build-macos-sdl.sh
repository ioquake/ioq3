#!/bin/bash

# This must be executed on macOS with xcode and cmake installed, obviously

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. ${SCRIPT_DIR}/../lib-versions.sh

ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

set -e

TMPDIR=$(mktemp -d)
cd "$TMPDIR"

SDL_TARBALL="SDL3-${SDL_VERSION}.tar.gz"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/${SDL_TARBALL}"

curl -sLO "$SDL_URL"

tar -xzf "$SDL_TARBALL"
SDL_DIR="SDL3-${SDL_VERSION}"
cd $TMPDIR/$SDL_DIR

BUILD_DIR="build"
cmake -S . -B ${BUILD_DIR} -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES:STRING="x86_64;arm64"
cmake --build ${BUILD_DIR} --parallel $(sysctl -n hw.ncpu)

DEST_DIR="${ROOT_DIR}/code/thirdparty/libs/macos/"
mkdir -p ${DEST_DIR}

cp ${BUILD_DIR}/libSDL3.dylib ${DEST_DIR}
install_name_tool -id @executable_path/libSDL3.dylib ${DEST_DIR}/libSDL3.dylib

cd
rm -rf "$TMPDIR"
