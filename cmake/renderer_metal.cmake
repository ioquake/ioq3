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
    ${SOURCE_DIR}/renderermetal/tr_backend.cpp
    ${SOURCE_DIR}/renderermetal/tr_init.cpp
    ${SOURCE_DIR}/renderermetal/tr_extensions.cpp
    ${SOURCE_DIR}/renderermetal/tr_extramath.cpp
    ${SOURCE_DIR}/renderermetal/tr_dsa.cpp
    ${SOURCE_DIR}/renderermetal/tr_scene.cpp
    ${SOURCE_DIR}/renderermetal/tr_shader.cpp
    ${SOURCE_DIR}/renderermetal/tr_texture.cpp
    ${SOURCE_DIR}/sdl/sdl_metal.cpp
)

set(RENDERER_METAL_BASENAME renderer_metal)
set(RENDERER_METAL_BINARY ${RENDERER_METAL_BASENAME})

list(APPEND RENDERER_METAL_BINARY_SOURCES
    ${RENDERER_METAL_SOURCES}
    ${RENDERER_COMMON_SOURCES}
    ${RENDERER_LIBRARY_SOURCES})


# Metal shader compilation
find_program(XCRUN xcrun REQUIRED)

# Metal shader sources
set(METAL_SHADER_DIR ${SOURCE_DIR}/renderermetal/shaders)
file(GLOB METAL_SHADER_SOURCES ${METAL_SHADER_DIR}/*.metal)

# Output directory for compiled shaders
set(METAL_SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/metal_shaders)
file(MAKE_DIRECTORY ${METAL_SHADER_OUTPUT_DIR})

# Compile each .metal file to .air
set(METAL_AIR_FILES "")
foreach(SHADER_SRC ${METAL_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SRC} NAME_WE)
    set(AIR_FILE ${METAL_SHADER_OUTPUT_DIR}/${SHADER_NAME}.air)
    
    add_custom_command(
        OUTPUT ${AIR_FILE}
        COMMAND ${XCRUN} -sdk macosx metal -c ${SHADER_SRC} -o ${AIR_FILE}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling Metal shader: ${SHADER_NAME}.metal"
        VERBATIM
    )
    
    list(APPEND METAL_AIR_FILES ${AIR_FILE})
endforeach()

# Link all .air files into default.metallib
set(METALLIB_FILE ${METAL_SHADER_OUTPUT_DIR}/default.metallib)
add_custom_command(
    OUTPUT ${METALLIB_FILE}
    COMMAND ${XCRUN} -sdk macosx metallib ${METAL_AIR_FILES} -o ${METALLIB_FILE}
    DEPENDS ${METAL_AIR_FILES}
    COMMENT "Creating Metal library: default.metallib"
    VERBATIM
)

# Create custom target for Metal shaders
add_custom_target(metal_shaders ALL DEPENDS ${METALLIB_FILE})

if(USE_RENDERER_DLOPEN)
    list(APPEND RENDERER_METAL_BINARY_SOURCES ${DYNAMIC_RENDERER_SOURCES})

    add_library(${RENDERER_METAL_BINARY} SHARED ${RENDERER_METAL_BINARY_SOURCES})
    
    # Make renderer depend on shader compilation
    add_dependencies(${RENDERER_METAL_BINARY} metal_shaders)

    target_link_libraries(      ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_LIBRARIES} "-framework Metal" "-framework QuartzCore" "-framework Foundation")
    target_include_directories( ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_INCLUDE_DIRS} ${SOURCE_DIR}/thirdparty/metal-cpp)
    target_compile_definitions( ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_DEFINITIONS})
    target_compile_options(     ${RENDERER_METAL_BINARY} PRIVATE ${RENDERER_COMPILE_OPTIONS})
    target_compile_features(    ${RENDERER_METAL_BINARY} PRIVATE cxx_std_17)

    set_output_dirs(${RENDERER_METAL_BINARY})
    
    # Copy metallib to app bundle Resources directory (where Metal looks for it)
    add_custom_command(TARGET ${RENDERER_METAL_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:ioquake3>/Resources
        COMMAND ${CMAKE_COMMAND} -E copy ${METALLIB_FILE} $<TARGET_BUNDLE_CONTENT_DIR:ioquake3>/Resources/default.metallib
        COMMENT "Copying default.metallib to app bundle Resources"
        VERBATIM
    )
endif()


