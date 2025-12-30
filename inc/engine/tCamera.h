#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "tVulkanDevice.h"

class tCamera
{
  public:
    tCamera(const tVulkanDevice &device, const vk::Extent2D extent);
    ~tCamera()
    {
        CameraMemory.unmapMemory();
        spdlog::info("tCamera: Destroyed");
    }

    void updateViewData();
    void addYawPitch(float deltaYaw, float deltaPitch);
    void moveLocal(const glm::vec3 &dir, float distance);
    void onResize(const vk::Extent2D extent);

    const vk::raii::DescriptorSet &getDescriptorSet() const { return DescriptorSet; }
    const vk::raii::DescriptorSetLayout &getDescriptorSetLayout() const { return DescriptorLayout; }

  private:
    struct tParams
    {
        glm::mat4 Projection;
        glm::mat4 View;
    };
    const size_t BufferSize = sizeof(tParams);

    void createCameraBuffer();
    void createDescriptorSet();
    void createDescriptorLayout();
    void updateDescriptorSet();

    void updateView();
    void updateProj(const vk::Extent2D &extent);

    const tVulkanDevice &Device;
    const vk::raii::Device &LogicalDevice{nullptr};
    const vk::raii::PhysicalDevice &PhysicalDevice{nullptr};

    vk::raii::DescriptorPool DescriptorPool{nullptr};
    vk::raii::DescriptorSetLayout DescriptorLayout{nullptr};
    vk::raii::DescriptorSet DescriptorSet{nullptr};

    vk::raii::Buffer CameraBuffer{nullptr};
    vk::raii::DeviceMemory CameraMemory{nullptr};

    glm::vec3 Position{0.0f, 0.0f, -10.0f};
    float Yaw{0.0f};
    float Pitch{0.f};
    float FovY{glm::radians(80.0f)};
    float ZNear{0.1f};
    float ZFar{500.f};

    tParams CachedUBO{};
    void *MappedCameraData;
};
