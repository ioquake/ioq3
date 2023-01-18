set(QCOMMON_SOURCES_armv7l
	"${QCOMMON_DIR}/vm_armv7l.c"
)
set(QCOMMON_SOURCES_arm ${QCOMMON_SOURCES_armv7l})
set(QCOMMON_SOURCES_arm64 ${QCOMMON_SOURCES_armv7l})
set(QCOMMON_SOURCES_x86
	"${QCOMMON_DIR}/vm_x86.c"
)
set(QCOMMON_SOURCES_x86_64 ${QCOMMON_SOURCES_x86})
set(QCOMMON_SOURCES_ppc
	"${QCOMMON_DIR}/vm_powerpc.c"
	"${QCOMMON_DIR}/vm_powerpc_asm.c"
)
set(QCOMMON_SOURCES_ppc64 ${QCOMMON_SOURCES_ppc})
set(QCOMMON_SOURCES_sparc
	"${QCOMMON_DIR}/vm_sparc.c"
)
set(QCOMMON_SOURCES_sparc64 ${QCOMMON_SOURCES_sparc})


macro(GET_VM_SUPPORT ARG_VAR)
	if(DEFINED QCOMMON_SOURCES_${ARCH})
		set(${ARG_VAR} TRUE)
	else()
		set(${ARG_VAR} FALSE)
	endif()
endmacro()

macro(GET_VM_SOURCES ARG_VAR)
	if(DEFINED QCOMMON_SOURCES_${ARCH})
		set(${ARG_VAR} ${QCOMMON_SOURCES_${ARCH}})
	endif()
endmacro()
