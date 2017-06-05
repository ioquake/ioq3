add_library (
    ogg STATIC
    include/ogg/config_types.h
    include/ogg/ogg.h
    include/ogg/os_types.h
    src/bitwise.c
    src/framing.c
    )

include_directories(include)
