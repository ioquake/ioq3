set( SPEEX_INCLUDES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include ) 

add_library (
    speex STATIC
    _kiss_fft_guts.h
    arch.h
    bits.c
    buffer.c
    cb_search.c
    cb_search.h
    cb_search_arm4.h
    cb_search_bfin.h
    cb_search_sse.h
    config.h
    exc_10_16_table.c
    exc_10_32_table.c
    exc_20_32_table.c
    exc_5_256_table.c
    exc_5_64_table.c
    exc_8_128_table.c
    fftwrap.c
    fftwrap.h
    filterbank.c
    filterbank.h
    filters.c
    filters.h
    filters_arm4.h
    filters_bfin.h
    filters_sse.h
    fixed_arm4.h
    fixed_arm5e.h
    fixed_bfin.h
    fixed_debug.h
    fixed_generic.h
    gain_table.c
    gain_table_lbr.c
    hexc_10_32_table.c
    hexc_table.c
    high_lsp_tables.c
    jitter.c
    kiss_fft.c
    kiss_fft.h
    kiss_fftr.c
    kiss_fftr.h
    lpc.c
    lpc.h
    lpc_bfin.h
    lsp.c
    lsp.h
    lsp_bfin.h
    lsp_tables_nb.c
    ltp.c
    ltp.h
    ltp_arm4.h
    ltp_bfin.h
    ltp_sse.h
    math_approx.h
    mdf.c
    misc_bfin.h
    modes.c
    modes.h
    modes_wb.c
    nb_celp.c
    nb_celp.h
    os_support.h
    preprocess.c
    pseudofloat.h
    quant_lsp.c
    quant_lsp.h
    quant_lsp_bfin.h
    resample.c
    sb_celp.c
    sb_celp.h
    smallft.c
    smallft.h
    speex.c
    speex_callbacks.c
    speex_header.c
    stack_alloc.h
    stereo.c
    testdenoise.c
    testecho.c
    testenc.c
    testenc_uwb.c
    testenc_wb.c
    vbr.c
    vbr.h
    vorbis_psy.h
    vq.c
    vq.h
    vq_arm4.h
    vq_bfin.h
    vq_sse.h
    window.c
)

add_definitions( -DFLOATING_POINT )

target_include_directories(
    speex PUBLIC
    ${SPEEX_INCLUDES_DIR}
    ) 

set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11" )
