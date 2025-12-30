#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_raii.hpp>

class tCamera;

class tSwapchain;

class tVulkanDevice;

class tGui
{
  public:
    tGui(tCamera &camera,
         const tVulkanDevice &device,
         const tSwapchain &swapchain,
         const vk::raii::Instance &instance,
         GLFWwindow &window);
    ~tGui() { spdlog::info("tGui: Destroyed"); }

    void update();
    void recordGuiPass(const vk::raii::CommandBuffer &commandBuffer,
                       const vk::Extent2D &extent,
                       const vk::Image &image,
                       const vk::ImageView &imageView) const;
    float getFrameRate() const { return Io->Framerate; };

  private:
    static constexpr float MouseSensitivity = 0.0025f;
    static constexpr float MoveSpeed = 7.0f;

    void updateFPSCounter();
    void handleCameraUserInputs();
    void handleCameraKeyboard(float deltaTime);
    void handleCameraMouse();

    void initImGui(const tVulkanDevice &device, const vk::raii::Instance &instance, const tSwapchain &swapchain);

    ImGuiIO *Io{nullptr};
    vk::raii::DescriptorPool ImGuiPool{nullptr};
    ImGui_ImplVulkanH_Window *ImGuiWindow{nullptr};

    tCamera &Camera;
    GLFWwindow &Window;

    double LastMousePosX, LastMousePosY;
    bool IsFirstMouse = true; // so we don't get a huge jump when re-entering
};
