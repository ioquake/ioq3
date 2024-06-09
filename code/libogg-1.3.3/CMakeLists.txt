set(SRCS
	src/bitwise.c
	src/framing.c
)
add_library(ogg STATIC ${SRCS})
target_include_directories(ogg PUBLIC include)
