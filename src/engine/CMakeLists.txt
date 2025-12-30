add_library(engine)

target_sources(engine
    PRIVATE
    tCamera.cpp
    tGui.cpp
    tRenderer.cpp
    tSwapchain.cpp
    tVulkanDevice.cpp
    tVulkanInstance.cpp
    tWindow.cpp
)

find_package(Vulkan REQUIRED)

target_link_libraries(engine
    PUBLIC
        TracyClient
        helpers
        Vulkan::Vulkan
        glfw
        glm
        dear_imgui
        X11
)

# Include directories
target_include_directories(engine
    PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
