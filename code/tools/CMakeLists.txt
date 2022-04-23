
if (MSVC)
	# 4456 variable shadowing
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4456 /wd4311 /wd4312 /wd4255 /wd4389 /wd4459 /wd4996 /wd4018 /wd4210 /wd4457 /wd4706 /wd4710 /wd4774 /wd4711")
endif()

add_executable(stringify stringify.c)

set(Q3ASM_SRCS
	asm/cmdlib.c
	asm/q3asm.c
)
add_executable(q3asm ${Q3ASM_SRCS})

set(LBURG_SRCS
	lcc/lburg/lburg.c
	lcc/lburg/gram.c
)
add_executable(lburg ${LBURG_SRCS})

set(Q3CPP_SRCS
	lcc/cpp/cpp.c
	lcc/cpp/lex.c
	lcc/cpp/nlist.c
	lcc/cpp/tokens.c
	lcc/cpp/macro.c
	lcc/cpp/eval.c
	lcc/cpp/include.c
	lcc/cpp/hideset.c
	lcc/cpp/getopt.c
	lcc/cpp/unix.c
)
add_executable(q3cpp ${Q3CPP_SRCS})

set(Q3RCC_SRCS
	dagcheck.c

	lcc/src/alloc.c
	lcc/src/bind.c
	lcc/src/bytecode.c
	lcc/src/dag.c
	lcc/src/decl.c
	lcc/src/enode.c
	lcc/src/error.c
	lcc/src/event.c
	lcc/src/expr.c
	lcc/src/gen.c
	lcc/src/init.c
	lcc/src/inits.c
	lcc/src/input.c
	lcc/src/lex.c
	lcc/src/list.c
	lcc/src/main.c
	lcc/src/null.c
	lcc/src/output.c
	lcc/src/prof.c
	lcc/src/profio.c
	lcc/src/simp.c
	lcc/src/stmt.c
	lcc/src/string.c
	lcc/src/sym.c
	lcc/src/symbolic.c
	lcc/src/trace.c
	lcc/src/tree.c
	lcc/src/types.c
)
add_executable(q3rcc ${Q3RCC_SRCS})
set_source_files_properties(dagcheck.c PROPERTIES GENERATED TRUE)
target_include_directories(q3rcc PRIVATE lcc/src)
add_custom_command(OUTPUT dagcheck.c
	COMMAND lburg ${CMAKE_CURRENT_SOURCE_DIR}/lcc/src/dagcheck.md dagcheck.c
	DEPENDS lburg
	COMMENT "lburg dagcheck.c"
)

set(Q3LCC_SRCS
	lcc/etc/lcc.c
	lcc/etc/bytecode.c
)
add_executable(q3lcc ${Q3LCC_SRCS})
add_dependencies(q3lcc q3cpp q3rcc)
