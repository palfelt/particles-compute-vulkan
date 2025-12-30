include(FetchContent)

message(STATUS "Fetching third-party dependencies...")

# ──────────────────────────────────────────────────────────────
# Tracy Profiler
# ──────────────────────────────────────────────────────────────
option(VULKAN_COMPUTE_ENABLE_TRACY "Build with Tracy profiler hooks" ON)

if(VULKAN_COMPUTE_ENABLE_TRACY)
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
else()
    set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG master
)
FetchContent_MakeAvailable(tracy)
set_target_properties(TracyClient PROPERTIES COMPILE_FLAGS "-w") # GCC/Clang

# ──────────────────────────────────────────────────────────────
# GLM - Math library
# ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glm)
set_target_properties(glm PROPERTIES COMPILE_FLAGS "-w") # GCC/Clang

# ──────────────────────────────────────────────────────────────
# GLFW - Windowing library
# ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS) 

# ──────────────────────────────────────────────────────────────
# Dear ImGui
# ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking  # 'docking' branch includes docking support
)
FetchContent_MakeAvailable(imgui)

# Create a library target for ImGui
add_library(dear_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp # Or SDL, adjust if needed
)
target_include_directories(dear_imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

set_target_properties(dear_imgui PROPERTIES COMPILE_FLAGS "-w") # GCC/Clang

# ──────────────────────────────────────────────────────────────

message(STATUS "Third-party dependencies ready.")
