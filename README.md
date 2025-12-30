# vulkan-compute

Naive particle simulation using Vulkan RAII + compute + mesh/task shaders. Tested on Ubuntu 24.04.3 LTS

![Demo](assets/demo.webp)

## Highlights

- Particle gravity simulation (`shaders/forceNaive.comp`)
- Dynamic rendering via task + mesh shaders
- Barebones `Dear ImGui` + `Tracy Profiler` +  `spdlog` integration

## Pre-requisites

- Vulkan 1.4, SPIR-V 1.6
- GPU with the `VK_EXT_mesh_shader` feature
- CMake 3.28+
- C++23 compiler
- `spdlog` (see below to install from source)
- Tests: `GTest`

To install `spdlog` from source, run the following from any directory:

```console
$ git clone https://github.com/gabime/spdlog.git
$ cd spdlog && mkdir build && cd build
$ cmake .. && cmake --build .
```

## Build

Use the provided `CMakePresets.json` to build the project.

`FetchContent` is used for for: Tracy, GLM, GLFW, Dear ImGui.

## Controls

- `WASD` + `Space` / `LeftCtrl`: move
- Hold `RMB`: mouse look

## License

Apache-2.0 (see `LICENSE`).
