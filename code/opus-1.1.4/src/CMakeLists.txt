# CMakeLists.txt in src

SET(OPUS_HEADERS
    ../include/opus.h
    ../include/opus_multistream.h
    analysis.h
    mlp.h
    opus_private.h
    tansig_table.h
    )

add_library (
    opus STATIC
    ${OPUS_HEADERS}
    analysis.c
    analysis.h
    mlp.c
    mlp.h
    mlp_data.c
    opus.c
    opus_decoder.c
    opus_encoder.c
    opus_multistream.c
    opus_multistream_decoder.c
    opus_multistream_encoder.c
    opus_private.h
    repacketizer.c
    tansig_table.h
    )

INCLUDE_DIRECTORIES(
    ${OPUS_INCLUDES_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../celt
    ${CMAKE_CURRENT_SOURCE_DIR}/../silk
    ${CMAKE_CURRENT_SOURCE_DIR}/../silk/fixed
    ${CMAKE_CURRENT_SOURCE_DIR}/../silk/float
    )

ADD_LIBRARY( opus STATIC ${OPUS_HEADERS} ${OPUS_SRCS})
