add_library (
    asm STATIC
    ftola.c
    qasm-inline.h
    qasm.h
    snapvector.c
)

set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11" )
