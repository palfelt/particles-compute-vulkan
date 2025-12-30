#pragma once

#include <vector>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class tVulkanDevice;

struct DepthBufferData
{
    vk::raii::Image Image{nullptr};
    vk::raii::DeviceMemory Memory{nullptr};
    vk::raii::ImageView ImageView{nullptr};
};

struct tSwapchain
{
    ~tSwapchain() { spdlog::info("tSwapchain: Destroyed"); }

    void init(const vk::raii::Instance &instance,
              const tVulkanDevice &device,
              const vk::raii::SurfaceKHR &surface,
              const vk::Extent2D extent);
    void recreate(vk::Extent2D extent);

    vk::Extent2D getExtent() const { return Extent; }
    const vk::Format &getColorFormat() const { return ColorFormat; }
    const vk::Format &getDepthFormat() const { return DepthFormat; }
    uint32_t getMinImageCount() const { return MinImageCount; }
    uint32_t getImageCount() const { return ImageViews.size(); }
    const vk::raii::ImageView &getImageView(size_t ix) const { return ImageViews[ix]; }
    const std::vector<vk::raii::ImageView> &getImageViews() const { return ImageViews; }
    const vk::Image &getImage(size_t ix) const { return Images[ix]; }
    const std::vector<vk::Image> &getImages() const { return Images; }
    const vk::raii::ImageView &getDepthImageView() const { return DepthBuffer.ImageView; }
    const vk::raii::SwapchainKHR &getSwapchain() const { return Swapchain; }

  private:
    void create(vk::Extent2D extent);
    void createSwapchain(vk::Extent2D extent);
    void createImageViews();
    void createDepthResources();

    uint32_t MinImageCount;
    std::vector<vk::Image> Images;
    std::vector<vk::raii::ImageView> ImageViews;
    DepthBufferData DepthBuffer;
    vk::raii::SwapchainKHR Swapchain{nullptr};

    const vk::raii::SurfaceKHR *Surface{nullptr};
    const vk::raii::Instance *Instance{nullptr};
    const vk::raii::Device *Device{nullptr};
    const vk::raii::PhysicalDevice *PhysicalDevice{nullptr};
    const vk::raii::Queue *Queue{nullptr};

    vk::Extent2D Extent{};
    vk::Format ColorFormat = vk::Format::eR8G8B8A8Unorm;
    vk::Format DepthFormat = vk::Format::eD16Unorm;
};
