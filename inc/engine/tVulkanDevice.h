#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
// vulkan-tracy include order
#include <tracy/TracyVulkan.hpp>

class tVulkanDevice
{
  public:
    void init(const vk::raii::Instance &instance, const vk::SurfaceKHR &surface, bool enableValidation);
    ~tVulkanDevice();

    vk::raii::CommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(vk::raii::CommandBuffer &commandBuffer) const;

    const vk::raii::PhysicalDevice &getPhysicalDevice() const { return PhysicalDevice; }
    const vk::raii::Device &getLogicalDevice() const { return Device; }
    const vk::raii::Queue &getQueue() const { return Queue; }
    uint32_t getQueueFamily() const { return QueueFamily; }
    const vk::raii::CommandPool &getCommandPool() const { return CommandPool; }
    TracyVkCtx getTracyContext() const { return TracyContext; }

  private:
    void pickPhysicalDevice(const vk::raii::Instance &instance);
    void createLogicalDevice();
    void createCommandPool();
    void initTracyContext();
    bool supportsRequiredFeaturesAndExtensions(vk::PhysicalDevice device) const;
    bool queueSupportsPresent(vk::PhysicalDevice device, uint32_t queueFamily, vk::SurfaceKHR Surface);

    vk::raii::PhysicalDevice PhysicalDevice{nullptr};
    vk::raii::Device Device{nullptr};
    vk::raii::CommandPool CommandPool{nullptr};
    vk::raii::Queue Queue{nullptr};
    vk::raii::CommandBuffer TracyCommandBuffer{nullptr};
    uint32_t QueueFamily = 0;
    vk::SurfaceKHR Surface = VK_NULL_HANDLE;

    TracyVkCtx TracyContext{nullptr};
    bool ValidationEnabled = false;
};
