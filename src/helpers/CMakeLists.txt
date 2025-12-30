add_library(helpers)

target_sources(helpers
    PRIVATE
    createBuffer.cpp
    loadShaders.cpp
    memoryAllocation.cpp
)

target_include_directories(helpers
    PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(helpers
    PUBLIC
        TracyClient
)

target_compile_definitions(helpers PRIVATE SHADER_DIR=\"${CMAKE_BINARY_DIR}/shaders/\")