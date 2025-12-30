add_library(sim)

target_sources(sim
    PRIVATE
    tPhysics.cpp
    tSim.cpp
)

target_link_libraries(sim
    PUBLIC
        TracyClient
        glm
        dear_imgui
        X11
)

# Include directories
target_include_directories(sim
    PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
