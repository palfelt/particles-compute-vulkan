# Source and output directories
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

# Automatically find all shader files
file(GLOB SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/*.vert
    ${SHADER_SOURCE_DIR}/*.frag
    ${SHADER_SOURCE_DIR}/*.comp
    ${SHADER_SOURCE_DIR}/*.mesh
    ${SHADER_SOURCE_DIR}/*.task
)

# List to hold the compiled outputs
set(SHADER_OUTPUTS)

# Loop through each shader and create its compilation rule
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

# Recursively find all shaders
file(GLOB_RECURSE SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/*.vert
    ${SHADER_SOURCE_DIR}/*.frag
    ${SHADER_SOURCE_DIR}/*.comp
    ${SHADER_SOURCE_DIR}/*.mesh
    ${SHADER_SOURCE_DIR}/*.task
)

set(SHADER_OUTPUTS)

foreach(SHADER ${SHADER_SOURCES})
    # Get relative path under shaders/
    file(RELATIVE_PATH REL_PATH ${SHADER_SOURCE_DIR} ${SHADER})
    # Replace extension with .spv appended (e.g. "post/blur.frag" → "post/blur.frag.spv")
    set(SPV_FILE ${SHADER_OUTPUT_DIR}/${REL_PATH}.spv)

    # Ensure output directory exists for this shader
    get_filename_component(SPV_DIR ${SPV_FILE} DIRECTORY)

    set(GLSLC_FLAGS
        --target-env=vulkan1.4
        --target-spv=spv1.6
    )

    if(SHADER_EXT STREQUAL ".mesh")
        list(APPEND GLSLC_FLAGS -fshader-stage=mesh)
    elseif(SHADER_EXT STREQUAL ".task")
        list(APPEND GLSLC_FLAGS -fshader-stage=task)
    elseif(SHADER_EXT STREQUAL ".vert")
        list(APPEND GLSLC_FLAGS -fshader-stage=vert)
    elseif(SHADER_EXT STREQUAL ".frag")
        list(APPEND GLSLC_FLAGS -fshader-stage=frag)
    elseif(SHADER_EXT STREQUAL ".comp")
        list(APPEND GLSLC_FLAGS -fshader-stage=comp)
    endif()

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SPV_DIR}
        COMMAND glslc ${GLSLC_FLAGS} ${SHADER} -o ${SPV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${REL_PATH}"
        VERBATIM
    )

    list(APPEND SHADER_OUTPUTS ${SPV_FILE})
endforeach()


# Define a target that builds all shaders
add_custom_target(shaders_spv DEPENDS ${SHADER_OUTPUTS})

# Symlink source shaders into the build folder for runtime access (optional)
add_custom_target(shader_symlinks ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
        TARGET shader_symlinks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${SHADER}
                ${SHADER_OUTPUT_DIR}/${SHADER_NAME}
        COMMENT "Symlinked ${SHADER_NAME} → ${SHADER_OUTPUT_DIR}"
        VERBATIM
    )
endforeach()

add_dependencies(shaders_spv shader_symlinks)
