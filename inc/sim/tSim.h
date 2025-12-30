#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "engine/tVulkanDevice.h"
#include "tPhysics.h"

class tSim
{
  public:
    tSim(const tVulkanDevice &device, uint32_t nrDescriptorSets);
    ~tSim() { spdlog::info("tSim: Destroyed"); }

    void recordComputePass(const vk::raii::CommandBuffer &commandBuffer, size_t ixImage) const;
    void updateParams(const tPhysics::tParams &physicsParams) { Physics->updateParams(physicsParams); };
    void swapParticleBuffers() { Physics->swapParticleBuffers(); };

    const vk::raii::Buffer &getParticleBuffer() const { return Physics->getBufferB(); }
    const vk::raii::DescriptorSet &getDescriptorSet(uint32_t ix) const { return DescriptorSets[ix]; }
    const vk::raii::DescriptorSetLayout &getDescriptorSetLayout() const { return DescriptorLayout; }

  private:
    const uint32_t NrDescriptorSets;

    void createDescriptorSets();
    void createDescriptorSetLayout();
    void updateDescriptorSetForFrame(const vk::raii::DescriptorSet &set) const;

    const tVulkanDevice &Device;
    const vk::raii::Device &LogicalDevice;
    const vk::raii::PhysicalDevice &PhysicalDevice;

    // std::unique_ptr<tCellGrid> CellGrid{nullptr};
    std::unique_ptr<tPhysics> Physics{nullptr};

    vk::raii::DescriptorSetLayout DescriptorLayout{nullptr};
    vk::raii::DescriptorPool DescriptorPool{nullptr};
    vk::raii::DescriptorSets DescriptorSets{nullptr};
};