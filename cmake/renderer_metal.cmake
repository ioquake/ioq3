if(NOT BUILD_CLIENT OR NOT BUILD_RENDERER_METAL)
    return()
endif()

if(NOT APPLE)
    message(STATUS "Metal renderer is only built on Apple platforms")
    return()
endif()

include(utils/set_output_dirs)
include(renderer_common)

set(RENDERER_METAL_SOURCES
    ${SOURCE_DIR}/renderermetal/tr_init.cpp
)

set(RENDERER_METAL_BASENAME renderer_metal)
set(RENDERER_METAL_BINARY ${RENDERER_METAL_BASENAME})

list(APPEND RENDERER_METAL_BINARY_SOURCES
    ${RENDERER_METAL_SOURCES}
    ${RENDERER_LIBRARY_SOURCES})

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_METAL_BINARY_SOURCES ${DYNAMIC_RENDERER_SOURCES})

    add_library(${RENDERER_METAL_BINARY} SHARED ${RENDERER_METAL_BINARY_SOURCES})

    target_link_libraries(      ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_LIBRARIES} "-framework Metal" "-framework QuartzCore" "-framework Foundation")
    target_include_directories( ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS} ${SOURCE_DIR}/thirdparty/metal-cpp)
    target_compile_definitions( ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_compile_features(    ${RENDERER_METAL_BINARY} PRIVATE cxx_std_17)

    set_output_dirs(${RENDERER_METAL_BINARY})
endif()
