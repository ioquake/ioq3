# MSVC compiler specific settings

if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    return()
endif()

include(utils/arch)

if(ARCH MATCHES "x86" OR ARCH MATCHES "x86_64")
    enable_language(ASM_MASM)

    set(ASM_SOURCES
        ${SOURCE_DIR}/asm/snapvector.asm
        ${SOURCE_DIR}/asm/ftola.asm
    )
endif()

if(ARCH MATCHES "x86_64")
    list(APPEND ASM_SOURCES ${SOURCE_DIR}/asm/vm_x86_64.asm)
    set_source_files_properties(
        ${ASM_SOURCES}
        PROPERTIES COMPILE_DEFINITIONS "idx64")
endif()

# Baseline warnings
add_compile_options("$<$<COMPILE_LANGUAGE:C>:/W4>")

# C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
# There are way too many of these to realistically deal with them
add_compile_options("$<$<COMPILE_LANGUAGE:C>:/wd4267>")

# MSVC doesn't understand __inline__, which libjpeg uses
add_compile_definitions(__inline__=inline)

# It's unlikely that we'll move to the _s variants, so stop the warning
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# The sockets platform abstraction layer necessarily uses deprecated APIs
add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
