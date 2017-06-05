# CMakeLists.txt in silk/float

SET(SILK_FLOAT_SRCS
    apply_sine_window_FLP.c
    autocorrelation_FLP.c
    burg_modified_FLP.c
    bwexpander_FLP.c
    corrMatrix_FLP.c
    encode_frame_FLP.c
    energy_FLP.c
    find_LPC_FLP.c
    find_LTP_FLP.c
    find_pitch_lags_FLP.c
    find_pred_coefs_FLP.c
    inner_product_FLP.c
    k2a_FLP.c
    levinsondurbin_FLP.c
    LPC_analysis_filter_FLP.c
    LPC_inv_pred_gain_FLP.c
    LTP_analysis_filter_FLP.c
    LTP_scale_ctrl_FLP.c
    noise_shape_analysis_FLP.c
    pitch_analysis_core_FLP.c
    prefilter_FLP.c
    process_gains_FLP.c
    regularize_correlations_FLP.c
    residual_energy_FLP.c
    scale_copy_vector_FLP.c
    scale_vector_FLP.c
    schur_FLP.c
    solve_LS_FLP.c
    sort_FLP.c
    warped_autocorrelation_FLP.c
    wrappers_FLP.c
    )

SET(SILK_FLOAT_HEADERS
    main_FLP.h
    SigProc_FLP.h
    structs_FLP.h
    )

INCLUDE_DIRECTORIES(
    ${OPUS_INCLUDES_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../celt
    )

