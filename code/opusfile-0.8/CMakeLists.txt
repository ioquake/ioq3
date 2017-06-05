add_library(
    opusfile STATIC
    include/opusfile.h
    src/http.c
    src/info.c
    src/internal.c
    src/internal.h
    src/opusfile.c
    src/stream.c
    src/wincerts.c
    src/winerrno.h
    )

include_directories(
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../opus-1.1.4/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libogg-1.3.2/include
    )

target_link_libraries(
    opusfile
    opus
    ogg
    )
