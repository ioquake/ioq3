#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CODE="${ROOT}/code"

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

    (
        cd ${CODE}/${DIR}
        [[ -n "$COMMAND" ]] && eval "$COMMAND"
        find . -type f -not -regex ${INCLUDE_PATTERN} -delete
        find . -type f -regex ${EXCLUDE_PATTERN} -delete
        find . -type d -empty -delete
    )
}

prepare "https://downloads.xiph.org/releases/ogg/libogg-1.3.6.tar.gz" "./configure" "\./\(include\|src\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz" "./configure" "\./\(include\|lib\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz" "./configure" "\./\(celt\|include\|silk\|src\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/opus/opusfile-0.12.tar.gz" "./configure" "\./\(include\|src\)/.*\.[ch]"
prepare "https://zlib.net/zlib-1.3.1.tar.gz" "./configure" "./[^/]*\.[ch]"
prepare "https://www.ijg.org/files/jpegsrc.v9f.tar.gz" "./configure" "./\(j.*\.c\|.*\.h\)" "./jmem\(ansi\|dos\|mac\|name\)\.c"
prepare "https://curl.se/download/curl-8.15.0.tar.gz" "./configure --with-openssl" "\.*/include/.*\.h"
