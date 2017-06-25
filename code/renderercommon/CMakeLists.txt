include(${CMAKE_SOURCE_DIR}/cmake/SDL2.cmake)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c ${SDL2_INCLUDE_DIRS} )

add_library(
    renderercommon STATIC
    iqm.h
    qgl.h
    tr_common.h
    tr_font.c
    tr_image_bmp.c
    tr_image_jpg.c
    tr_image_pcx.c
    tr_image_png.c
    tr_image_tga.c
    tr_noise.c
    tr_public.h
    tr_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jaricom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcapimin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcapistd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcarith.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jccoefct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jccolor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcdctmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jchuff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcinit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcmainct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcmarker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcmaster.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcomapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcparam.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcprepct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jcsample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jctrans.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdapimin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdapistd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdarith.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdatadst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdatasrc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdcoefct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdcolor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jddctmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdhuff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdinput.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdmainct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdmarker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdmaster.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdmerge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdpostct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdsample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jdtrans.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jerror.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jfdctflt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jfdctfst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jfdctint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jidctflt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jidctfst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jidctint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jmemmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jmemnobs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jquant1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jquant2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../jpeg-8c/jutils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdl/sdl_gamma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdl/sdl_glimp.c
    )

add_definitions(
    -DFLOATING_POINT
    -DHAVE_LRINTF
    -DUSE_INTERNAL_JPEG
    -DUSE_RENDERER_DLOPEN
    ${SDL2_DEFINES}
    )

target_link_libraries( renderercommon ${SDL2_LIBRARIES} )

#command /usr/bin/clang -fPIC -fno-common -Isrc/lua-5.3.3/include -Isrc/script/rapidjson  -Isrc/nettle-3.3 -fno-strict-aliasing -fno-common -pipe -D_THREAD_SAFE=1 -Isrc/SDL2/include -DNO_GZIP -Isrc/zlib -DUSE_INTERNAL_JPEG -Isrc/jpeg-8c -DUSE_LOCAL_HEADERS -DSTANDALONE -DPRODUCT_VERSION=\1.2.0-39f5d1c-2016-11-04\ -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization -Werror-implicit-function-declaration -MMD -I/usr/local/include -DUSE_OPENAL -DUSE_OPENAL_DLOPEN -DUSE_CURL -DUSE_VOIP -DUSE_CODEC_OPUS -DOPUS_BUILD -DHAVE_LRINTF -DFLOATING_POINT -DUSE_ALLOCA -Isrc/opus-1.1/include -Isrc/opus-1.1/celt -Isrc/opus-1.1/silk -Isrc/opus-1.1/silk/float -Isrc/opusfile-0.5/include -DUSE_RESTCLIENT -Isrc/restclient -Isrc/libogg-1.3.1/include -DUSE_RENDERER_DLOPEN -DUSE_MUMBLE -DNDEBUG  -arch x86_64 -mfpmath=sse -ffast-math -o build/release-darwin-x86_64/renderergl1/tr_font.o -c src/renderercommon/tr_font.c
