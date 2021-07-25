set(SRCS
	lib/lsp.c
	lib/vorbisenc.c
	lib/analysis.c
	lib/floor0.c
	lib/lpc.c
	lib/codebook.c
	lib/envelope.c
	lib/mapping0.c
	lib/vorbisfile.c
	lib/bitrate.c
	lib/sharedbook.c
	lib/smallft.c
	lib/info.c
	lib/mdct.c
	lib/lookup.c
	lib/floor1.c
	lib/block.c
	lib/registry.c
	lib/synthesis.c
	lib/window.c
	lib/psy.c
	lib/res0.c
)
add_library(vorbis STATIC ${SRCS})
target_link_libraries(vorbis PUBLIC ogg)
target_include_directories(vorbis PUBLIC include lib)
target_compile_definitions(vorbis PUBLIC USE_CODEC_VORBIS)
