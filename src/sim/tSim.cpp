#include "sim/tSim.h"

#include <tracy/Tracy.hpp>

tSim::tSim(const tVulkanDevice &device, const uint32_t nrDescriptorSets)
    : Device(device), LogicalDevice(device.getLogicalDevice()), PhysicalDevice(device.getPhysicalDevice()),
      NrDescriptorSets(nrDescriptorSets)
{
    spdlog::info("tSim: Initializing...");
    createDescriptorSetLayout();
    createDescriptorSets();
    Physics = std::make_unique<tPhysics>(Device, DescriptorLayout);
    spdlog::info("tSim: Initialized");
}

void tSim::recordComputePass(const vk::raii::CommandBuffer &commandBuffer, const size_t ixImage) const
{
    ZoneScopedN("tSim: recordComputePass()");
    spdlog::trace("tSim: Recording compute pass at index {}...", ixImage);
    const auto &set = DescriptorSets[ixImage];
    updateDescriptorSetForFrame(set);

    Physics->recordPhysicsPass(commandBuffer, set);
    spdlog::trace("tSim: Recorded compute pass");
}

void tSim::createDescriptorSets()
{
    spdlog::info("tSim: Creating {} descriptor sets...", NrDescriptorSets);
    vk::DescriptorPoolSize paramsPoolSize{vk::DescriptorType::eUniformBuffer, 1u * NrDescriptorSets};
    vk::DescriptorPoolSize buffersPoolSize{vk::DescriptorType::eStorageBuffer, 2u * NrDescriptorSets};
    std::array poolSizes{paramsPoolSize, buffersPoolSize};
    vk::DescriptorPoolCreateInfo dpci{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                      NrDescriptorSets,
                                      static_cast<uint32_t>(poolSizes.size()),
                                      poolSizes.data()};
    DescriptorPool = vk::raii::DescriptorPool(LogicalDevice, dpci);

    std::vector<vk::DescriptorSetLayout> layouts(NrDescriptorSets, *DescriptorLayout);
    vk::DescriptorSetAllocateInfo dsai{*DescriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data()};
    DescriptorSets = vk::raii::DescriptorSets(LogicalDevice, dsai);
    spdlog::info("tSim: {} Descriptor sets created", DescriptorSets.size());
}

void tSim::createDescriptorSetLayout()
{
    spdlog::info("tSim: Creating {} descriptor set layout...", NrDescriptorSets);
    const auto simStages = vk::ShaderStageFlagBits::eCompute;
    vk::DescriptorSetLayoutBinding physicsParamsBinding{0, vk::DescriptorType::eUniformBuffer, 1, simStages};
    vk::DescriptorSetLayoutBinding readParticlesBinding{1, vk::DescriptorType::eStorageBuffer, 1, simStages};
    vk::DescriptorSetLayoutBinding writeParticlesBinding{2, vk::DescriptorType::eStorageBuffer, 1, simStages};
    std::array bindings{physicsParamsBinding, readParticlesBinding, writeParticlesBinding};

    vk::DescriptorSetLayoutCreateInfo dslci({}, bindings);
    DescriptorLayout = LogicalDevice.createDescriptorSetLayout(dslci);
    spdlog::info("tSim: Creating {} descriptor set layout...", NrDescriptorSets);
}

void tSim::updateDescriptorSetForFrame(const vk::raii::DescriptorSet &set) const
{
    ZoneScopedN("tSim: updateDescriptorSetForFrame()");
    spdlog::trace("tSim: Updating the descriptor sets for the current frame...");
    vk::DescriptorBufferInfo simInfo{Physics->getParamsBuffer(), 0, sizeof(tPhysics::tParams)};
    vk::DescriptorBufferInfo readInfo{Physics->getBufferA(), 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo writeInfo{Physics->getBufferB(), 0, VK_WHOLE_SIZE};

    std::array writes{vk::WriteDescriptorSet{*set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &simInfo},
                      vk::WriteDescriptorSet{*set, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &readInfo},
                      vk::WriteDescriptorSet{*set, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &writeInfo}};
    LogicalDevice.updateDescriptorSets(writes, {});
    spdlog::trace("tSim: Updated the descriptor sets for the current frame");
}
