#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CODE="${ROOT}/code"

prepare()
{
    local URL="$1"
    shift
    local EXCLUDE_PATTERNS=("$@")
    local FILENAME=$(basename ${URL})

    local EXCLUDE_ARGS= 
    for EXCLUDE_PATTERN in "${EXCLUDE_PATTERNS[@]}"; do
        EXCLUDE_ARGS+=" -not -regex ${EXCLUDE_PATTERN}"
    done

    echo ${FILENAME}

    local EXTRACT_LOG=$(mktemp)
    curl -sL "${URL}" | tar -xvz -C "${CODE}" | tee "${EXTRACT_LOG}"
    local DIR=$(head -n1 "${EXTRACT_LOG}")

    (
        cd ${CODE}/${DIR}
        [ -f ./configure ] && ./configure
        find . -type f ${EXCLUDE_ARGS[@]} -delete
        find . -type d -empty -delete
    )
}

prepare "https://downloads.xiph.org/releases/ogg/libogg-1.3.6.tar.gz" "\./\(include\|src\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz" "\./\(include\|lib\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz" "\./\(celt\|include\|silk\|src\)/.*\.[ch]"
prepare "https://downloads.xiph.org/releases/opus/opusfile-0.12.tar.gz" "\./\(include\|src\)/.*\.[ch]"
prepare "https://zlib.net/zlib-1.3.1.tar.gz" "./[^/]*\.[ch]"
prepare "https://www.ijg.org/files/jpegsrc.v9f.tar.gz" "./[^/]*\.[ch]"
