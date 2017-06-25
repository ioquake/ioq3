
include(CMakeParseArguments)

set(QVM_TOOLS_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qvm_tools)
set(Q3CPP_BINARY ${QVM_TOOLS_DIR}/q3cpp )
set(Q3RCC_BINARY ${QVM_TOOLS_DIR}/q3rcc )
set(Q3LCC_BINARY ${QVM_TOOLS_DIR}/q3lcc )
set(Q3ASM_BINARY ${QVM_TOOLS_DIR}/q3asm )

set(QVM_DEPS ${Q3ASM_BINARY} ${Q3CPP_BINARY} ${Q3LCC_BINARY} ${Q3RCC_BINARY})

macro(QVM_COMPILE_ASM defs outfile infile)
    add_custom_command(
        OUTPUT  ${outfile}
        COMMAND ${Q3LCC_BINARY}
        ARGS    ${defs} -o ${outfile} ${infile}
        DEPENDS ${QVM_DEPS} 
        )
    set_source_files_properties(${outfile} PROPERTIES GENERATED TRUE)
endmacro()

macro(ADD_QVM Name)
    set(options MISSIONPACK)
    cmake_parse_arguments(ADD_QVM "${options}" "" "" ${ARGN})

    string(TOUPPER ${Name} UPNAME)

    if( ${ADD_QVM_MISSIONPACK} )
        set(defs "-DVMS" "-D${UPNAME}" "-DMISSIONPACK")
    else( ${ADD_QVM_MISSIONPACK} )
        set(defs "-DVMS" "-D${UPNAME}")
    endif( ${ADD_QVM_MISSIONPACK} )

    foreach(srcfile ${ADD_QVM_UNPARSED_ARGUMENTS})
        #message( "> ${srcfile}" )

        get_filename_component(ext ${srcfile} EXT) 
        if ("${ext}" STREQUAL ".asm")
            set(outfile ${CMAKE_CURRENT_SOURCE_DIR}/${srcfile})
            # Add asm files directly
            list(APPEND srcs ${outfile})
        endif()

        if ("${ext}" STREQUAL ".c")
            get_filename_component(outfile ${srcfile} NAME_WE)
            set(outfile ${outfile}.asm)
            # compile C code into asm
            qvm_compile_asm("${defs}" "${outfile}" "${CMAKE_CURRENT_SOURCE_DIR}/${srcfile}")
            # add asm to list
            list(APPEND srcs ${outfile})
        endif()

        if ("${ext}" STREQUAL ".h")
            # XXX: Ignore headers??
        endif()
    endforeach()

    #message("SOURCES <${srcs}>")

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${Name}.qvm
        COMMAND ${Q3ASM_BINARY}
        ARGS -o ${Name}.qvm ${srcs}
        DEPENDS ${Q3ASM_BINARY} ${srcs}
        )
    add_custom_target(${Name}.qvm DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${Name}.qvm)
endmacro()
#
#============================================================

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(QVM 
    REQUIRED_VARS Q3LCC_BINARY Q3ASM_BINARY 
    )
