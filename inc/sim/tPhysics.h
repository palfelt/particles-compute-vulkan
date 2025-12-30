#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
// vulkan-tracy include order
#include <tracy/TracyVulkan.hpp>

class tVulkanDevice;

class tPhysics
{
  public:
    tPhysics(const tVulkanDevice &device, const vk::raii::DescriptorSetLayout &descriptorLayout);
    ~tPhysics() { spdlog::info("tPhysics: Destroyed"); }

    struct tParams
    {
        float DeltaTime;
    };

    void swapParticleBuffers();
    void updateParams(const tParams &params);
    void recordPhysicsPass(const vk::raii::CommandBuffer &commandBuffer, const vk::raii::DescriptorSet &set) const;

    const vk::raii::Buffer &getBufferA() const { return ParticleBufferA; }
    const vk::raii::Buffer &getBufferB() const { return ParticleBufferB; }
    const vk::raii::Buffer &getParamsBuffer() const { return ParamsBuffer; }

  private:
    uint32_t LocalSize = 128;

    const tVulkanDevice &Device;
    const vk::raii::Device &LogicalDevice;
    const vk::raii::PhysicalDevice &PhysicalDevice;

    const TracyVkCtx TracyContext;

    void createBuffers();
    void createPhysicsPipeline(const vk::raii::DescriptorSetLayout &setLayout);
    void createShaderModules();

    vk::raii::Pipeline PhysicsPipeline{nullptr};
    vk::raii::PipelineLayout PhysicsPipelineLayout{nullptr};

    vk::raii::Buffer ParticleBufferA{nullptr};
    vk::raii::DeviceMemory ParticleMemoryA{nullptr};
    vk::raii::Buffer ParticleBufferB{nullptr};
    vk::raii::DeviceMemory ParticleMemoryB{nullptr};
    vk::raii::Buffer ParamsBuffer{nullptr};
    vk::raii::DeviceMemory ParamsMemory{nullptr};

    vk::raii::ShaderModule PhysicsShader{nullptr};

    tParams CachedParams{};
    void *MappedParamsData;
};