#  _______ ___ ___ _______                              __ __
# |       |   |   |   |   | .----.-----.--------.-----.|__|  |.-----.----.
# |   -  _|   |   |       | |  __|  _  |        |  _  ||  |  ||  -__|   _|
# |_______|\_____/|__|_|__| |____|_____|__|__|__|   __||__|__||_____|__|
#                                               |__|
# TODO: DO NOT build unless building a QVM


#  _  _                     
# | || |                    
# | || |          ,_    __, 
# |/ |/ \_|   |  /  |  /  | 
# |__/\_/  \_/|_/   |_/\_/|/
#                        /| 
#                        \| 

add_executable(
    lburg
    lcc/lburg/lburg.c
    lcc/lburg/gram.c
    )


#        ___                      
#       /   \                     
#  __,    __/ __,   ,   _  _  _   
# /  |      \/  |  / \_/ |/ |/ |  
# \_/|_/\___/\_/|_/ \/   |  |  |_/
#    |\                           
#    |/                           
# 

add_executable(
    q3asm
    asm/q3asm.c
    asm/cmdlib.c
    )

target_include_directories( q3asm PUBLIC lcc/src lcc/lburg )


#        ___  _            
#       /   \| |           
#  __,    __/| |  __   __  
# /  |      \|/  /    /    
# \_/|_/\___/|__/\___/\___/
#    |\                    
#    |/                    
# 

add_executable(
    q3lcc
    lcc/etc/bytecode.c
    lcc/etc/lcc.c
    )

target_include_directories( q3lcc PUBLIC lcc/src lcc/lburg )

#       ___                
#      /   \               
# __,    __/ __    _    _  
#/  |      \/    |/ \_|/ \_
#\_/|_/\___/\___/|__/ |__/ 
#   |\          /|   /|    
#   |/          \|   \|    
#

add_executable(
    q3cpp
    #
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

target_include_directories( q3cpp PUBLIC lcc/src lcc/lburg )

#        ___                 
#       /   \                
#  __,    __/ ,_    __   __  
# /  |      \/  |  /    /    
# \_/|_/\___/   |_/\___/\___/
#    |\                      
#    |/                      
#

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c
    COMMAND lburg
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/lcc/src/dagcheck.md dagcheck.c
    DEPENDS lburg lcc/src/dagcheck.md
    )
set_source_files_properties(dagcheck.c PROPERTIES GENERATED TRUE)

add_executable(
    q3rcc
    lcc/src/alloc.c
    lcc/src/bind.c
    lcc/src/bytecode.c
    lcc/src/dag.c
    ${CMAKE_CURRENT_BINARY_DIR}/dagcheck.c
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

target_include_directories( q3rcc PUBLIC lcc/src lcc/lburg )

set_property(
    TARGET lburg q3asm q3lcc q3cpp q3rcc
    PROPERTY RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qvm_tools
    )

# Victor Roemer (wtfbbqhax), <victor@badsec.org>.
cmake_minimum_required(VERSION 3.5)
project(QVMCompiler C)
