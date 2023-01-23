# The MIT License (MIT)
# 
# Copyright (c) 2022 github.com/Pan7
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
cmake_minimum_required(VERSION 2.8.12)

project("internal opus")

set(OPUS_TARGET "internal_opus")

if(NOT DEFINED SOURCE_DIR)
	set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
	set(CMAKELIST_DIR "${SOURCE_DIR}/misc/CMakeLists")
endif()

include("${CMAKELIST_DIR}/include/cflags.cmake")
include("${CMAKELIST_DIR}/include/compat.cmake")
include("${CMAKELIST_DIR}/include/default_options.cmake")
include("${CMAKELIST_DIR}/include/directory_constants.cmake")

include(CheckCSourceCompiles)
include(CheckIncludeFile)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	option(GENERATE_DEPENDENCIES "Preprocessing: Generate dependencies for make outputting" ${DEFAULT_GENERATE_DEPENDENCIES})
endif()
if(MSVC)
	option(BUILD_STATIC_CRT "Build (static) multi-threaded C runtime library" ${DEFAULT_BUILD_STATIC_CRT})
endif()

set(OPUS_SOURCES
	"${OPUS_DIR}/src/analysis.c"
	"${OPUS_DIR}/src/mlp.c"
	"${OPUS_DIR}/src/mlp_data.c"
	"${OPUS_DIR}/src/opus.c"
	"${OPUS_DIR}/src/opus_decoder.c"
	"${OPUS_DIR}/src/opus_encoder.c"
	"${OPUS_DIR}/src/opus_multistream.c"
	"${OPUS_DIR}/src/opus_multistream_encoder.c"
	"${OPUS_DIR}/src/opus_multistream_decoder.c"
	"${OPUS_DIR}/src/repacketizer.c"

	"${OPUS_DIR}/celt/bands.c"
	"${OPUS_DIR}/celt/celt.c"
	"${OPUS_DIR}/celt/cwrs.c"
	"${OPUS_DIR}/celt/entcode.c"
	"${OPUS_DIR}/celt/entdec.c"
	"${OPUS_DIR}/celt/entenc.c"
	"${OPUS_DIR}/celt/kiss_fft.c"
	"${OPUS_DIR}/celt/laplace.c"
	"${OPUS_DIR}/celt/mathops.c"
	"${OPUS_DIR}/celt/mdct.c"
	"${OPUS_DIR}/celt/modes.c"
	"${OPUS_DIR}/celt/pitch.c"
	"${OPUS_DIR}/celt/celt_encoder.c"
	"${OPUS_DIR}/celt/celt_decoder.c"
	"${OPUS_DIR}/celt/celt_lpc.c"
	"${OPUS_DIR}/celt/quant_bands.c"
	"${OPUS_DIR}/celt/rate.c"
	"${OPUS_DIR}/celt/vq.c"

	"${OPUS_DIR}/silk/CNG.c"
	"${OPUS_DIR}/silk/code_signs.c"
	"${OPUS_DIR}/silk/init_decoder.c"
	"${OPUS_DIR}/silk/decode_core.c"
	"${OPUS_DIR}/silk/decode_frame.c"
	"${OPUS_DIR}/silk/decode_parameters.c"
	"${OPUS_DIR}/silk/decode_indices.c"
	"${OPUS_DIR}/silk/decode_pulses.c"
	"${OPUS_DIR}/silk/decoder_set_fs.c"
	"${OPUS_DIR}/silk/dec_API.c"
	"${OPUS_DIR}/silk/enc_API.c"
	"${OPUS_DIR}/silk/encode_indices.c"
	"${OPUS_DIR}/silk/encode_pulses.c"
	"${OPUS_DIR}/silk/gain_quant.c"
	"${OPUS_DIR}/silk/interpolate.c"
	"${OPUS_DIR}/silk/LP_variable_cutoff.c"
	"${OPUS_DIR}/silk/NLSF_decode.c"
	"${OPUS_DIR}/silk/NSQ.c"
	"${OPUS_DIR}/silk/NSQ_del_dec.c"
	"${OPUS_DIR}/silk/PLC.c"
	"${OPUS_DIR}/silk/shell_coder.c"
	"${OPUS_DIR}/silk/tables_gain.c"
	"${OPUS_DIR}/silk/tables_LTP.c"
	"${OPUS_DIR}/silk/tables_NLSF_CB_NB_MB.c"
	"${OPUS_DIR}/silk/tables_NLSF_CB_WB.c"
	"${OPUS_DIR}/silk/tables_other.c"
	"${OPUS_DIR}/silk/tables_pitch_lag.c"
	"${OPUS_DIR}/silk/tables_pulses_per_block.c"
	"${OPUS_DIR}/silk/VAD.c"
	"${OPUS_DIR}/silk/control_audio_bandwidth.c"
	"${OPUS_DIR}/silk/quant_LTP_gains.c"
	"${OPUS_DIR}/silk/VQ_WMat_EC.c"

	"${OPUS_DIR}/silk/HP_variable_cutoff.c"
	"${OPUS_DIR}/silk/NLSF_encode.c"
	"${OPUS_DIR}/silk/NLSF_VQ.c"
	"${OPUS_DIR}/silk/NLSF_unpack.c"
	"${OPUS_DIR}/silk/NLSF_del_dec_quant.c"
	"${OPUS_DIR}/silk/process_NLSFs.c"
	"${OPUS_DIR}/silk/stereo_LR_to_MS.c"
	"${OPUS_DIR}/silk/stereo_MS_to_LR.c"
	"${OPUS_DIR}/silk/check_control_input.c"
	"${OPUS_DIR}/silk/control_SNR.c"
	"${OPUS_DIR}/silk/init_encoder.c"
	"${OPUS_DIR}/silk/control_codec.c"
	"${OPUS_DIR}/silk/A2NLSF.c"
	"${OPUS_DIR}/silk/ana_filt_bank_1.c"
	"${OPUS_DIR}/silk/biquad_alt.c"
	"${OPUS_DIR}/silk/bwexpander_32.c"
	"${OPUS_DIR}/silk/bwexpander.c"
	"${OPUS_DIR}/silk/debug.c"
	"${OPUS_DIR}/silk/decode_pitch.c"
	"${OPUS_DIR}/silk/inner_prod_aligned.c"
	"${OPUS_DIR}/silk/lin2log.c"
	"${OPUS_DIR}/silk/log2lin.c"
	"${OPUS_DIR}/silk/LPC_analysis_filter.c"
	"${OPUS_DIR}/silk/LPC_fit.c"
	"${OPUS_DIR}/silk/LPC_inv_pred_gain.c"
	"${OPUS_DIR}/silk/table_LSF_cos.c"
	"${OPUS_DIR}/silk/NLSF2A.c"
	"${OPUS_DIR}/silk/NLSF_stabilize.c"
	"${OPUS_DIR}/silk/NLSF_VQ_weights_laroia.c"
	"${OPUS_DIR}/silk/pitch_est_tables.c"
	"${OPUS_DIR}/silk/resampler.c"
	"${OPUS_DIR}/silk/resampler_down2_3.c"
	"${OPUS_DIR}/silk/resampler_down2.c"
	"${OPUS_DIR}/silk/resampler_private_AR2.c"
	"${OPUS_DIR}/silk/resampler_private_down_FIR.c"
	"${OPUS_DIR}/silk/resampler_private_IIR_FIR.c"
	"${OPUS_DIR}/silk/resampler_private_up2_HQ.c"
	"${OPUS_DIR}/silk/resampler_rom.c"
	"${OPUS_DIR}/silk/sigm_Q15.c"
	"${OPUS_DIR}/silk/sort.c"
	"${OPUS_DIR}/silk/sum_sqr_shift.c"
	"${OPUS_DIR}/silk/stereo_decode_pred.c"
	"${OPUS_DIR}/silk/stereo_encode_pred.c"
	"${OPUS_DIR}/silk/stereo_find_predictor.c"
	"${OPUS_DIR}/silk/stereo_quant_pred.c"

	"${OPUS_DIR}/silk/float/apply_sine_window_FLP.c"
	"${OPUS_DIR}/silk/float/autocorrelation_FLP.c"
	"${OPUS_DIR}/silk/float/burg_modified_FLP.c"
	"${OPUS_DIR}/silk/float/bwexpander_FLP.c"
	"${OPUS_DIR}/silk/float/corrMatrix_FLP.c"
	"${OPUS_DIR}/silk/float/encode_frame_FLP.c"
	"${OPUS_DIR}/silk/float/energy_FLP.c"
	"${OPUS_DIR}/silk/float/find_LPC_FLP.c"
	"${OPUS_DIR}/silk/float/find_LTP_FLP.c"
	"${OPUS_DIR}/silk/float/find_pitch_lags_FLP.c"
	"${OPUS_DIR}/silk/float/find_pred_coefs_FLP.c"
	"${OPUS_DIR}/silk/float/inner_product_FLP.c"
	"${OPUS_DIR}/silk/float/k2a_FLP.c"
#	"${OPUS_DIR}/silk/float/levinsondurbin_FLP.c"
	"${OPUS_DIR}/silk/float/LPC_analysis_filter_FLP.c"
	"${OPUS_DIR}/silk/float/LPC_inv_pred_gain_FLP.c"
	"${OPUS_DIR}/silk/float/LTP_analysis_filter_FLP.c"
	"${OPUS_DIR}/silk/float/LTP_scale_ctrl_FLP.c"
	"${OPUS_DIR}/silk/float/noise_shape_analysis_FLP.c"
	"${OPUS_DIR}/silk/float/pitch_analysis_core_FLP.c"
#	"${OPUS_DIR}/silk/float/prefilter_FLP.c"
	"${OPUS_DIR}/silk/float/process_gains_FLP.c"
	"${OPUS_DIR}/silk/float/regularize_correlations_FLP.c"
	"${OPUS_DIR}/silk/float/residual_energy_FLP.c"
	"${OPUS_DIR}/silk/float/scale_copy_vector_FLP.c"
	"${OPUS_DIR}/silk/float/scale_vector_FLP.c"
	"${OPUS_DIR}/silk/float/schur_FLP.c"
	"${OPUS_DIR}/silk/float/sort_FLP.c"
#	"${OPUS_DIR}/silk/float/solve_LS_FLP.c"
	"${OPUS_DIR}/silk/float/warped_autocorrelation_FLP.c"
	"${OPUS_DIR}/silk/float/wrappers_FLP.c"
)
set(OPUS_HEADERS
	"${OPUS_DIR}/include/opus.h"
	"${OPUS_DIR}/include/opus_custom.h"
	"${OPUS_DIR}/include/opus_defines.h"
	"${OPUS_DIR}/include/opus_multistream.h"
	"${OPUS_DIR}/include/opus_types.h"

	"${OPUS_DIR}/src/analysis.h"
	"${OPUS_DIR}/src/mlp.h"
	"${OPUS_DIR}/src/opus_private.h"
	"${OPUS_DIR}/src/tansig_table.h"

	"${OPUS_DIR}/celt/_kiss_fft_guts.h"
	"${OPUS_DIR}/celt/arch.h"
	"${OPUS_DIR}/celt/bands.h"
	"${OPUS_DIR}/celt/celt.h"
	"${OPUS_DIR}/celt/celt_lpc.h"
	"${OPUS_DIR}/celt/cpu_support.h"
	"${OPUS_DIR}/celt/cwrs.h"
	"${OPUS_DIR}/celt/ecintrin.h"
	"${OPUS_DIR}/celt/entcode.h"
	"${OPUS_DIR}/celt/entdec.h"
	"${OPUS_DIR}/celt/entenc.h"
	"${OPUS_DIR}/celt/fixed_debug.h"
	"${OPUS_DIR}/celt/fixed_generic.h"
	"${OPUS_DIR}/celt/float_cast.h"
	"${OPUS_DIR}/celt/kiss_fft.h"
	"${OPUS_DIR}/celt/laplace.h"
	"${OPUS_DIR}/celt/mathops.h"
	"${OPUS_DIR}/celt/mdct.h"
	"${OPUS_DIR}/celt/mfrngcod.h"
	"${OPUS_DIR}/celt/modes.h"
	"${OPUS_DIR}/celt/os_support.h"
	"${OPUS_DIR}/celt/pitch.h"
	"${OPUS_DIR}/celt/quant_bands.h"
	"${OPUS_DIR}/celt/rate.h"
	"${OPUS_DIR}/celt/stack_alloc.h"
	"${OPUS_DIR}/celt/static_modes_fixed.h"
	"${OPUS_DIR}/celt/static_modes_float.h"
	"${OPUS_DIR}/celt/vq.h"

	"${OPUS_DIR}/silk/API.h"
	"${OPUS_DIR}/silk/control.h"
	"${OPUS_DIR}/silk/debug.h"
	"${OPUS_DIR}/silk/define.h"
	"${OPUS_DIR}/silk/errors.h"
	"${OPUS_DIR}/silk/Inlines.h"
	"${OPUS_DIR}/silk/MacroCount.h"
	"${OPUS_DIR}/silk/MacroDebug.h"
	"${OPUS_DIR}/silk/macros.h"
	"${OPUS_DIR}/silk/main.h"
	"${OPUS_DIR}/silk/pitch_est_defines.h"
	"${OPUS_DIR}/silk/PLC.h"
	"${OPUS_DIR}/silk/resampler_private.h"
	"${OPUS_DIR}/silk/resampler_rom.h"
	"${OPUS_DIR}/silk/resampler_structs.h"
	"${OPUS_DIR}/silk/SigProc_FIX.h"
	"${OPUS_DIR}/silk/structs.h"
	"${OPUS_DIR}/silk/tables.h"
	"${OPUS_DIR}/silk/tuning_parameters.h"
	"${OPUS_DIR}/silk/typedef.h"

	"${OPUS_DIR}/silk/float/main_FLP.h"
	"${OPUS_DIR}/silk/float/SigProc_FLP.h"
	"${OPUS_DIR}/silk/float/structs_FLP.h"

)

CHECK_INCLUDE_FILE("malloc.h" HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
	list(APPEND OPUS_DEFINITIONS "-DHAVE_MALLOC_H")
endif(HAVE_MALLOC_H)
CHECK_INCLUDE_FILE("alloca.h" HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
	list(APPEND OPUS_DEFINITIONS "-DHAVE_ALLOCA_H")
endif(HAVE_ALLOCA_H)

CHECK_C_SOURCE_COMPILES("#include <alloca.h>\n int main () {int foo=10;int *array = alloca(foo);return 0;}" HAVE_ALLOCA_IN_ALLOCA_H)
CHECK_C_SOURCE_COMPILES("#include <stdlib.h>\n int main () {int foo=10;int *array = alloca(foo);return 0;}" HAVE_ALLOCA_IN_STDLIB_H)
CHECK_C_SOURCE_COMPILES("#include <malloc.h>\n int main () {int foo=10;int *array = alloca(foo);return 0;}" HAVE_ALLOCA_IN_MALLOC_H)
if(USE_ALLOCA OR HAVE_ALLOCA_IN_ALLOCA_H OR HAVE_ALLOCA_IN_STDLIB_H OR HAVE_ALLOCA_IN_MALLOC_H)
	list(APPEND OPUS_DEFINITIONS "-DUSE_ALLOCA")
endif()

CHECK_C_SOURCE_COMPILES("#include <math.h>\n int main () {int value = lrintf (3.1415);return 0;}" HAVE_LRINTF)
if(HAVE_LRINTF)
	list(APPEND OPUS_DEFINITIONS "-DHAVE_LRINTF")
endif(HAVE_LRINTF)
set(CMAKE_REQUIRED_LIBRARIES "m")
CHECK_C_SOURCE_COMPILES("#include <math.h>\n int main () {int value = lrintf (3.1415);return 0;}" HAVE_LRINTF_IN_M)
set(CMAKE_REQUIRED_LIBRARIES)
if(HAVE_LRINTF_IN_M)
	list(APPEND OPUS_DEFINITIONS "-DHAVE_LRINTF")
	list(APPEND OPUS_LIBRARIES "m")
endif(HAVE_LRINTF_IN_M)

if(MSVC)
	#Enable Intrinsic Functions /Oi
	list(APPEND OPUS_CFLAGS $<$<CONFIG:Release>:/Oi>)

	#Disable specific warnings
	#list(APPEND OPUS_CFLAGS "/wd4996")

endif(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")

	set(WARNING_CFLAGS "-Wall" "-W" "-Wstrict-prototypes" "-Wextra" "-Wcast-align" "-Wnested-externs" "-Wshadow")
	ADD_SUPPORTED_CFLAGS(WARNING_CFLAGS OPUS_CFLAGS)

	set(OFLAGS "-O4" "-O3" "-O2" "-O")
	set(OFLAGS_SUPPORTED)
	ADD_FIRST_SUPPORTED_CFLAG(OFLAGS OFLAGS_SUPPORTED)
	list(APPEND OPUS_CFLAGS $<$<CONFIG:Release>:${OFLAGS_SUPPORTED}>)

	set(DFLAGS "-g")
	set(DFLAGS_SUPPORTED)
	ADD_SUPPORTED_CFLAGS(DFLAGS DFLAGS_SUPPORTED)
	list(APPEND OPUS_CFLAGS $<$<CONFIG:Debug>:${DFLAGS_SUPPORTED}>)

endif()

ADD_INTERNAL_DEFINITIONS(OPUS_DEFINITIONS)
ADD_INTERNAL_CFLAGS(OPUS_CFLAGS)

list(APPEND OPUS_INCLUDE_DIRS "${OPUS_DIR}/include")
list(APPEND OPUS_INCLUDE_DIRS "${OPUS_DIR}/celt")
list(APPEND OPUS_INCLUDE_DIRS "${OPUS_DIR}/silk")
list(APPEND OPUS_INCLUDE_DIRS "${OPUS_DIR}/silk/float")
list(APPEND OPUS_INCLUDE_DIRS "${OPUS_DIR}/src")

list(APPEND OPUS_DEFINITIONS "-DOPUS_BUILD")
list(APPEND OPUS_DEFINITIONS "-DFLOAT_APPROX")

source_group("Source Files" FILES ${OPUS_SOURCES})
source_group("Header Files" FILES ${OPUS_HEADERS})

add_library(${OPUS_TARGET} STATIC ${OPUS_SOURCES} ${OPUS_HEADERS})

target_compile_definitions(${OPUS_TARGET} PRIVATE ${OPUS_DEFINITIONS})

target_include_directories(${OPUS_TARGET} PRIVATE ${OPUS_INCLUDE_DIRS})

compat_target_compile_options(${OPUS_TARGET} PRIVATE ${OPUS_CFLAGS})

list(APPEND OPUS_LIBRARIES ${OPUS_TARGET})

set(INTERNAL_OPUS_DEFINITIONS "" PARENT_SCOPE)
set(INTERNAL_OPUS_INCLUDE_DIRS "" PARENT_SCOPE)
set(INTERNAL_OPUS_LIBRARIES "${OPUS_LIBRARIES}" PARENT_SCOPE)
