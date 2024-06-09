include(ExternalProject)

project(autosyncerator)

set(TFMVER 0.13.1)
set(LTCVER 1.17)

find_program(MAKE_EXE NAMES gmake nmake make)

ExternalProject_Add(
    tomsfastmath
    URL https://github.com/libtom/tomsfastmath/releases/download/v${TFMVER}/tfm-${TFMVER}.tar.xz
    DOWNLOAD_NAME tfm-${TFMVER}.tar.xz
    URL_HASH SHA256=47c97a1ada3ccc9fcbd2a8a922d5859a84b4ba53778c84c1d509c1a955ac1738
    STEP_TARGETS build
    BUILD_COMMAND ${MAKE_EXE}
)

ExternalProject_Get_Property(tomsfastmath source_dir)
set(TOMSFASTMATH_INCLUDE_DIRS ${source_dir}/src/headers)

ExternalProject_Add(
    tomscrypt
    URL https://github.com/libtom/libtomcrypt/releases/download/$LTCVER/crypt-${LTCVER}.tar.bz2
    DOWNLOAD_NAME crypt-${LTCVER}.tar.bz2
    URL_HASH SHA256=e33b47d77a495091c8703175a25c8228aff043140b2554c08a3c3cd71f79d116
    DEPENDS tomsfastmath-build
    BUILD_COMMAND ${MAKE_EXE} CFLAGS=-I${TOMSFASTMATH_INCLUDE_DIRS}
)

ExternalProject_Get_Property(tomscrypt source_dir)
set(TOMSCRYPT_INCLUDE_DIRS ${source_dir}/src/headers)

add_executable(${PROJECT_NAME} autoupdater.c)

target_include_directories(${PROJECT_NAME} PRIVATE ${TOMSFASTMATH_INCLUDE_DIRS} ${TOMSFASTMATH_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} curl)
