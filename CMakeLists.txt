cmake_minimum_required(VERSION 3.28.3)

project(vulkan-compute VERSION 0.1.0 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

# Tests (optional)
include(CTest)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    add_subdirectory(tst)
endif()

# Sources
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${SPV_SHADERS})

# Subdirectories
add_subdirectory(src)
add_subdirectory(shaders)
add_subdirectory(third_party)

add_dependencies(${PROJECT_NAME} shaders_spv)

# Compile options
target_compile_options(${PROJECT_NAME}
    PRIVATE
    -O2
    -Wall
    -Wextra
    -Wconversion
    -Werror
)

# Linker options
target_link_options(${PROJECT_NAME}
    PRIVATE
    -ldl
    -lpthread
    -lX11
    -lXxf86vm
    -lXrandr
    -lXi
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
    ${GLM_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    app
    spdlog::spdlog
    $<$<BOOL:${MINGW}>:ws2_32>
)
