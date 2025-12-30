#include "engine/tWindow.h"

#include <spdlog/spdlog.h>

tWindow::tWindow(const int width, const int height, const std::string_view &name) : Width{width}, Height{height}
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Window dimensions must be positive");
    }
    spdlog::info("tWindow: Initializing...");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    Window = glfwCreateWindow(Width, Height, name.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(Window, this);
    glfwSetFramebufferSizeCallback(Window, resizeCallback);
    spdlog::info("tWindow: Initialized");
}

tWindow::~tWindow()
{
    glfwDestroyWindow(Window);
    glfwTerminate();
    spdlog::info("tWindow: Destroyed");
}

void tWindow::createWindowSurface(const vk::raii::Instance &instance)
{
    spdlog::info("tWindow: Creating window surface...");
    VkSurfaceKHR tmp;
    VkResult res = glfwCreateWindowSurface(*instance, Window, nullptr, &tmp);
    if (res != VK_SUCCESS)
    {
        spdlog::error("tWindow: glfwCreateWindowSurface() failed with {}", vk::to_string(static_cast<vk::Result>(res)));
        throw std::runtime_error("tWindow: Failed to create window surface");
    }

    Surface = vk::raii::SurfaceKHR(instance, tmp);
    spdlog::info("tWindow: Created window surface");
}

void tWindow::resizeCallback(GLFWwindow *glfwWindow, const int width, const int height)
{
    auto window = reinterpret_cast<tWindow *>(glfwGetWindowUserPointer(glfwWindow));
    window->IsResized = true;
    spdlog::info("tWindow: Called resize. Old resolution: {}x{}. New Resolution: {}x{}",
                 window->Width,
                 window->Height,
                 width,
                 height);
    window->Width = width;
    window->Height = height;
}