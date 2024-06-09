set(SRCS
	src/http.c
	src/info.c
	src/internal.c
	src/opusfile.c
	src/stream.c
	src/wincerts.c
)
add_library(opusfile STATIC ${SRCS})
target_include_directories(opusfile PUBLIC include)
target_link_libraries(opusfile PRIVATE opus)
