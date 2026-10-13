# GNU style (GCC/Clang) compiler specific settings

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang$")
    return()
endif()

enable_language(ASM)

set(ASM_SOURCES
    ${SOURCE_DIR}/asm/ftola.c
    ${SOURCE_DIR}/asm/snapvector.c
)

add_compile_options(-Wall -Wimplicit -Wshadow
    -Wstrict-prototypes -Wformat=2  -Wformat-security
    -Wstrict-aliasing=2 -Wmissing-format-attribute
    -Wdisabled-optimization -Werror-implicit-function-declaration)

add_compile_options(-Wno-format-zero-length -Wno-format-nonliteral)

# There are lots of instances of union based aliasing in the code
# that rely on the compiler not optimising them away, so disable it
add_compile_options(-fno-strict-aliasing)

# This is necessary to hide all symbols unless explicitly exported
# via the Q_EXPORT macro
add_compile_options(-fvisibility=hidden)

# ppc64le: target POWER8 as baseline (the ppc64le minimum), which enables
# VSX/Altivec.  PPC64LE_CPU can raise the baseline (power9, power10,
# native) for machine-specific builds.  Big-endian ppc64 is left at the
# compiler default so builds keep working on pre-POWER8 CPUs
# (970/G5, POWER5-7).
include(utils/arch)
if(ARCH STREQUAL "ppc64" AND CMAKE_C_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    set(PPC64LE_CPU "power8" CACHE STRING
        "-mcpu baseline for ppc64le builds (power8, power9, power10, native)")
    add_compile_options(-mcpu=${PPC64LE_CPU})
    if(PPC64LE_CPU STREQUAL "power8")
        # keep POWER8 compatibility but schedule for POWER9 cores
        add_compile_options(-mtune=power9)
    endif()
endif()
