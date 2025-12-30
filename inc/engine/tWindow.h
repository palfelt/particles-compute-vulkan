#pragma once

#define GLFW_INCLUDE_VULKAN
#include <string_view>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class tWindow
{
  public:
    tWindow(int width, int height, const std::string_view &name);
    ~tWindow();

    tWindow(const tWindow &) = delete;
    tWindow &operator=(const tWindow &) = delete;

    const vk::Extent2D getExtent() const { return {static_cast<uint32_t>(Width), static_cast<uint32_t>(Height)}; }
    const vk::raii::SurfaceKHR &getSurface() const { return Surface; }
    GLFWwindow &getWindow() const { return *Window; }

    void resetResizedFlag() { IsResized = false; }
    void createWindowSurface(const vk::raii::Instance &instance);

    bool shouldClose() { return glfwWindowShouldClose(Window) != 0; }
    bool wasResized() const { return IsResized; }

  private:
    static void resizeCallback(GLFWwindow *glfwWindow, int width, int height);

    vk::raii::SurfaceKHR Surface{nullptr};

    int Width;
    int Height;
    bool IsResized = false;

    GLFWwindow *Window;
};