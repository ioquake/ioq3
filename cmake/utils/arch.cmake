include_guard(GLOBAL)

set(archdetect_c_code "
#if defined(_WIN64) || defined(__WIN64__)
#  if defined(__x86_64__) || defined(_M_X64)
#    error cmake_ARCH x86_64
#  endif
#elif defined(_WIN32) || defined(__WIN32__)
#  if defined( _M_IX86 ) || defined( __i386__ )
#    error cmake_ARCH x86
#  endif
#endif
#if defined(__APPLE__) || defined(__APPLE_CC__)
#  ifdef __ppc__
#    error cmake_ARCH ppc
#  elif defined __i386__
#    error cmake_ARCH x86
#  elif defined __x86_64__
#    error cmake_ARCH x86_64
#  elif defined __aarch64__
#    error cmake_ARCH arm64
#  endif
#endif
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__)
#  if defined(__x86_64__) || defined(__amd64__)
#   error cmake_ARCH  x86_64
#  elif defined(__i386__)
#   error cmake_ARCH  x86
#  elif defined(__aarch64__)
#   error cmake_ARCH  arm64
#  elif defined(__arm__)
#   error cmake_ARCH  arm
#  elif defined(__powerpc64__) || defined(__ppc64__)
#   error cmake_ARCH  ppc64
#  elif defined(__powerpc__) || defined(__ppc__)
#   error cmake_ARCH  ppc
#  elif defined(__alpha__)
#   error cmake_ARCH  alpha
#  endif
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#  ifdef __i386__
#    error cmake_ARCH x86
#  elif defined __amd64__
#    error cmake_ARCH x86_64
#  elif defined __axp__
#    error cmake_ARCH alpha
#  endif
#endif
#ifdef __sun
#  ifdef __i386__
#    error cmake_ARCH x86
#  elif defined __sparc
#    error cmake_ARCH sparc
#  endif
#endif
#ifdef __sgi
#  error cmake_ARCH mips
#endif
#ifdef __EMSCRIPTEN__
#  error cmake_ARCH wasm32
#endif
")

set(DETECT_ARCH_C ${CMAKE_BINARY_DIR}/detect_arch.c)

# Set ppc_support to TRUE before including this file or ppc and ppc64
# will be treated as invalid architectures since they are no longer supported by Apple

if((APPLE AND DEFINED CMAKE_OSX_ARCHITECTURES) AND NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    # On OS X we use CMAKE_OSX_ARCHITECTURES *if* it was set
    # First let's normalize the order of the values

    # Note that it's not possible to compile PowerPC applications if you are using
    # the OS X SDK version 10.6 or later - you'll need 10.4/10.5 for that, so we
    # disable it by default
    # See this page for more information:
    # http://stackoverflow.com/questions/5333490/how-can-we-restore-ppc-ppc64-as-well-as-full-10-4-10-5-sdk-support-to-xcode-4

    # Architecture defaults to i386 or ppc on OS X 10.5 and earlier, depending on the CPU type detected at runtime.
    # On OS X 10.6+ the default is x86_64 if the CPU supports it, i386 otherwise.

    foreach(osx_arch ${CMAKE_OSX_ARCHITECTURES})
        if("${osx_arch}" STREQUAL "ppc" AND ppc_support)
            set(osx_arch_ppc TRUE)
        elseif("${osx_arch}" STREQUAL "ppc64" AND ppc_support)
            set(osx_arch_ppc64 TRUE)
        elseif("${osx_arch}" STREQUAL "i386")
            set(osx_arch_i386 TRUE)
        elseif("${osx_arch}" STREQUAL "x86_64")
            set(osx_arch_x86_64 TRUE)
        elseif("${osx_arch}" STREQUAL "arm64")
            set(osx_arch_arm64 TRUE)
        else()
            message(FATAL_ERROR "Invalid OS X arch name: ${osx_arch}")
        endif()
    endforeach()

    # Now add all the architectures in our normalized order
    if(osx_arch_ppc)
        list(APPEND ARCH ppc)
    endif()

    if(osx_arch_ppc64)
        list(APPEND ARCH ppc64)
    endif()

    if(osx_arch_i386)
        list(APPEND ARCH i386)
    endif()

    if(osx_arch_x86_64)
        list(APPEND ARCH x86_64)
    endif()

    if(osx_arch_arm64)
        list(APPEND ARCH arm64)
    endif()

    message(STATUS "Target macOS architecture(s): ${ARCH}")

    return()
endif()

file(WRITE ${DETECT_ARCH_C} "${archdetect_c_code}")

# Detect the architecture in a rather creative way...
# This compiles a small C program which is a series of ifdefs that selects a
# particular #error preprocessor directive whose message string contains the
# target architecture. The program will always fail to compile (both because
# file is not a valid C program, and obviously because of the presence of the
# #error preprocessor directives... but by exploiting the preprocessor in this
# way, we can detect the correct target architecture even when cross-compiling,
# since the program itself never needs to be run (only the compiler/preprocessor)

if(DEFINED CMAKE_OSX_ARCHITECTURES)
	set(TA_CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES})
else()
	set(TA_CMAKE_FLAGS "")
endif()

# Just compile it.
# Instead, trigger a custom compile error message containing only the target architecture name
# So the target platform is known.
try_compile(
    _
    SOURCES ${DETECT_ARCH_C}
    OUTPUT_VARIABLE PROGRAM_OUTPUT
    CMAKE_FLAGS ${TA_CMAKE_FLAGS}
)

# Parse the architecture name from the compiler output
string(REGEX MATCH "cmake_ARCH ([a-zA-Z0-9_]+)" _ ${PROGRAM_OUTPUT})

set(ARCH ${CMAKE_MATCH_1})

if(NOT ARCH)
    message(FATAL_ERROR "No architecture detected")
endif()

message(STATUS "Target architecture: ${ARCH}")
