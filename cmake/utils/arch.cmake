include_guard(GLOBAL)

set(DETECT_ARCH_C ${CMAKE_BINARY_DIR}/detect_arch.c)

if(USE_ARCHLESS_FILENAMES)
    list(APPEND DETECT_ARCH_DEFINITIONS -DUSE_ARCHLESS_FILENAMES)
endif()

file(WRITE ${DETECT_ARCH_C}
"#include \"${SOURCE_DIR}/qcommon/q_platform.h\"
#pragma message(\"@\" ARCH_STRING \"@\")
int main(void) { return 0; }
")

try_compile(COMPILE_SUCCESS
    SOURCES ${DETECT_ARCH_C}
    COMPILE_DEFINITIONS ${DETECT_ARCH_DEFINITIONS}
    OUTPUT_VARIABLE COMPILE_OUTPUT)

string(REGEX MATCH "^[^\@]+@([a-zA-Z0-9_]+)@.*$" HAVE_MATCH ${COMPILE_OUTPUT})
set(ARCH ${CMAKE_MATCH_1})

if(NOT COMPILE_SUCCESS OR RUN_EXITCODE OR NOT HAVE_MATCH OR NOT ARCH)
    message(FATAL_ERROR "Architecture detection failed")
endif()
