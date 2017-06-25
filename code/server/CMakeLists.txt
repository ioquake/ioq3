
#
##    .dMMMb  dMMMMMP dMMMMb  dMP dMP dMMMMMP dMMMMb
##   dMP" VP dMP     dMP.dMP dMP dMP dMP     dMP.dMP
##   VMMMb  dMMMP   dMMMMK" dMP dMP dMMMP   dMMMMK"
## dP .dMP dMP     dMP"AMF  YMvAP" dMP     dMP"AMF
## VMMMP" dMMMMMP dMP dMP    VP"  dMMMMMP dMP dMP
#

add_definitions(
    -DDEDICATED
    -DUSE_LOCAL_HEADERS
    -DPRODUCT_VERSION="1.2.0 pre-release"
    -DUSE_VOIP
    -DNDEBUG
    )

set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(APPLE)
set(APPLE_SOURCES ${PARENT_DIR}/sys/sys_osx.m)
endif(APPLE)

add_executable(
    ioq3ded 
    #
    server.h
    #
    sv_bot.c
    sv_ccmds.c
    sv_client.c
    sv_game.c
    sv_init.c
    sv_main.c
    sv_net_chan.c
    sv_snapshot.c
    sv_world.c
    #
    ${PARENT_DIR}/qcommon/cm_load.c
    ${PARENT_DIR}/qcommon/cm_patch.c
    ${PARENT_DIR}/qcommon/cm_polylib.c
    ${PARENT_DIR}/qcommon/cm_test.c
    ${PARENT_DIR}/qcommon/cm_trace.c
    ${PARENT_DIR}/qcommon/cmd.c
    ${PARENT_DIR}/qcommon/common.c
    ${PARENT_DIR}/qcommon/cvar.c
    ${PARENT_DIR}/qcommon/files.c
    ${PARENT_DIR}/qcommon/huffman.c
    ${PARENT_DIR}/qcommon/ioapi.c
    ${PARENT_DIR}/qcommon/md4.c
    ${PARENT_DIR}/qcommon/msg.c
    ${PARENT_DIR}/qcommon/net_chan.c
    ${PARENT_DIR}/qcommon/net_ip.c
    ${PARENT_DIR}/qcommon/q_math.c
    ${PARENT_DIR}/qcommon/q_shared.c
    ${PARENT_DIR}/qcommon/unzip.c
    ${PARENT_DIR}/qcommon/vm.c
    ${PARENT_DIR}/qcommon/vm_interpreted.c
    ${PARENT_DIR}/qcommon/vm_x86.c
    #
    ${PARENT_DIR}/null/null_client.c
    ${PARENT_DIR}/null/null_input.c
    ${PARENT_DIR}/null/null_snddma.c
    #
    ${PARENT_DIR}/asm/ftola.c
    ${PARENT_DIR}/asm/snapvector.c
    #
    ${PARENT_DIR}/sys/con_log.c
    ${PARENT_DIR}/sys/con_tty.c
    ${PARENT_DIR}/sys/sys_main.c
    ${PARENT_DIR}/sys/sys_unix.c
    ${APPLE_SOURCES}
    #
    ${PARENT_DIR}/zlib/adler32.c
    ${PARENT_DIR}/zlib/crc32.c
    ${PARENT_DIR}/zlib/inffast.c
    ${PARENT_DIR}/zlib/inflate.c
    ${PARENT_DIR}/zlib/inftrees.c
    ${PARENT_DIR}/zlib/zutil.c
    )

if(APPLE)
 set(FRAMEWORKS "-framework Cocoa -framework Security -framework OpenAL -framework IOKit")
else(APPLE)
 if(UNIX)
  set(SYSLIBS dl rt m)
 endif(UNIX)
endif(APPLE)

target_link_libraries(
    ioq3ded 
    #
    botlib
    #
    zlib
    ${FRAMEWORKS}
    ${SYSLIBS}
    )
    
include_directories(
       ${PARENT_DIR}/zlib
    )
