#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. ${SCRIPT_DIR}/lib-versions.sh

ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CODE="${ROOT}/code/thirdparty"

prepare()
{
    local URL="$1"
    local COMMAND="$2"
    local INCLUDE_PATTERN="$3"
    local EXCLUDE_PATTERN="$4"

    local FILENAME=$(basename ${URL})

    echo ${FILENAME}

    local EXTRACT_LOG=$(mktemp)
    curl -sL "${URL}" | tar -xvz -C "${CODE}" | tee "${EXTRACT_LOG}"
    local DIR=$(head -n1 "${EXTRACT_LOG}")

    echo $DIR
    if [ ! -d "${CODE}/${DIR}" ]; then
        DIR=$(dirname "$DIR")
    fi
    echo $DIR

    (
        cd ${CODE}/${DIR}
        [[ -n "$COMMAND" ]] && eval "$COMMAND"
        find . -type f -not -regex ${INCLUDE_PATTERN} -delete
        find . -type f -regex ${EXCLUDE_PATTERN} -delete
        find . -type d -empty -delete
        find . -xtype l -delete
    )
}

prepare "https://downloads.xiph.org/releases/ogg/libogg-${OGG_VERSION}.tar.gz" \
    "./configure" \
    "\./\(include\|src\)/.*\.[ch]"

prepare "https://downloads.xiph.org/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.gz" \
    "./configure" \
    "\./\(include\|lib\)/.*\.[ch]" \
    "\./lib/\(barkmel\|psytune\|tone\|vorbisenc\)\.c"

prepare "https://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz" \
    "./configure" \
    "\./\(celt\|include\|silk\|src\)/.*\.[ch]" \
    "\./.*\(arm\|_compare\|_demo\|fixed\|mapping_matrix\|mips\|opus_projection_\|tests\|x86\).*"

prepare "https://downloads.xiph.org/releases/opus/opusfile-${OPUSFILE_VERSION}.tar.gz" \
    "./configure" \
    "\./\(include\|src\)/.*\.[ch]"

prepare "https://zlib.net/zlib-${ZLIB_VERSION}.tar.gz" \
    "" \
    "\./[^/]*\.[ch]" \
    "\./\(compress\|deflate\|infback\|trees\|uncompr\|gz.*\)\.[c]"

prepare "https://www.ijg.org/files/jpegsrc.v${JPEG_VERSION}.tar.gz" \
    "./configure" \
    "\./\(j.*\.c\|.*\.h\)" \
    "\./\(jmem\(ansi\|dos\|mac\|name\)\|jpegtran\)\.c"

prepare "https://curl.se/download/curl-${CURL_VERSION}.tar.gz" \
    "./configure --with-openssl" \
    "\.*/include/.*\.h"

prepare "https://github.com/kcat/openal-soft/archive/refs/tags/${OPENAL_VERSION}.tar.gz" \
    "" \
    "\./include/AL/.*\.h"

prepare "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz" \
    "" \
    "\./include/.*\.h"

TMPDIR=$(mktemp -d)
cd ${TMPDIR}

curl -sL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-mingw.tar.gz" | tar -xvz
curl -sL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-VC.zip" \
    -o temp.zip && unzip -o temp.zip && rm temp.zip

cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x86/SDL2.dll ${CODE}/libs/win32/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x86/SDL2.lib ${CODE}/libs/win32/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x86/SDL2main.lib ${CODE}/libs/win32/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/i686-w64-mingw32/lib/libSDL2.dll.a ${CODE}/libs/win32/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/i686-w64-mingw32/lib/libSDL2main.a ${CODE}/libs/win32/

cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x64/SDL2.dll ${CODE}/libs/win64/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x64/SDL2.lib ${CODE}/libs/win64/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/lib/x64/SDL2main.lib ${CODE}/libs/win64/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/x86_64-w64-mingw32/lib/libSDL2.dll.a ${CODE}/libs/win64/
cp ${TMPDIR}/SDL2-${SDL_VERSION}/x86_64-w64-mingw32/lib/libSDL2main.a ${CODE}/libs/win64/

cd
rm -r ${TMPDIR}
