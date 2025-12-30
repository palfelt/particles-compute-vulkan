#include "sim/tPhysics.h"

#include <random>

#include <tracy/Tracy.hpp>

#include "engine/tVulkanDevice.h"
#include "helpers/createBuffer.h"
#include "helpers/loadShaders.h"
#include "sim/constants.h"
#include "sim/tParticle.h"

tPhysics::tPhysics(const tVulkanDevice &device, const vk::raii::DescriptorSetLayout &descriptorLayout)
    : Device(device), LogicalDevice(device.getLogicalDevice()), PhysicalDevice(device.getPhysicalDevice()),
      TracyContext(device.getTracyContext())
{
    spdlog::info("tPhysics: Initializing...");
    createShaderModules();
    createPhysicsPipeline(descriptorLayout);
    createBuffers();
    spdlog::info("tPhysics: Initialized");
}

void tPhysics::swapParticleBuffers()
{
    spdlog::trace("tPhysics: Swapping read/write particle buffers...");
    std::swap(ParticleBufferA, ParticleBufferB);
    spdlog::trace("tPhysics: Swapped read/write particle buffers");
}

void tPhysics::updateParams(const tPhysics::tParams &params)
{
    ZoneScopedN("tPhysics: updateParams()");
    spdlog::trace("tPhysics: Updating params...");
    CachedParams.DeltaTime = params.DeltaTime;
    std::memcpy(MappedParamsData, &CachedParams, sizeof(tParams));
    spdlog::trace("tPhysics: Updated params");
}

void tPhysics::recordPhysicsPass(const vk::raii::CommandBuffer &commandBuffer, const vk::raii::DescriptorSet &set) const
{
    ZoneScopedN("tPhysics: recordPhysicsPass()");
    spdlog::trace("tPhysics: Recording physics pass...");
    TracyVkNamedZone(TracyContext, tracyPhysicsZone, *commandBuffer, "Physics Dispatch", true);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, PhysicsPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, PhysicsPipelineLayout, 0, *set, {});
    const uint32_t dispatchX = (NUM_PARTICLES + LocalSize - 1) / LocalSize;
    commandBuffer.dispatch(dispatchX, 1, 1);
    spdlog::trace("tPhysics: Recorded compute pass");
}

void tPhysics::createBuffers()
{
    spdlog::info("tPhysics: Creating buffers...");
    std::tie(ParamsBuffer, ParamsMemory, MappedParamsData) =
        createBuffer(Device,
                     sizeof(tParams),
                     vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::SharingMode::eExclusive,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     nullptr);

    std::vector<tParticle> particles(NUM_PARTICLES);
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    for (auto &p : particles)
    {
        glm::vec3 pos = glm::normalize(glm::vec3(dist(rng) * 2.0f, dist(rng) * 1.4f, dist(rng))) * 2.0f;
        glm::vec3 vel = glm::normalize(glm::cross(pos, glm::vec3(0.9, 2.0, 0.7))) * 0.2f;
        p.Position = glm::vec4(pos, 1.0f);
        p.Velocity = glm::vec4(vel, 0.0f);
    }

    std::tie(ParticleBufferA, ParticleMemoryA, std::ignore) =
        createBuffer(Device,
                     NUM_PARTICLES * sizeof(tParticle),
                     vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::SharingMode::eExclusive,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     particles.data());

    std::tie(ParticleBufferB, ParticleMemoryB, std::ignore) =
        createBuffer(Device,
                     NUM_PARTICLES * sizeof(tParticle),
                     vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::SharingMode::eExclusive,
                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                     nullptr);
    spdlog::info("tPhysics: Buffers created");
}

void tPhysics::createPhysicsPipeline(const vk::raii::DescriptorSetLayout &setLayout)
{
    spdlog::info("tPhysics: Creating compute pipeline...");
    vk::PipelineLayoutCreateInfo plci({}, *setLayout);
    PhysicsPipelineLayout = LogicalDevice.createPipelineLayout(plci);

    vk::PipelineShaderStageCreateInfo stageInfo({}, vk::ShaderStageFlagBits::eCompute, PhysicsShader, "main");
    vk::ComputePipelineCreateInfo cpci({}, stageInfo, PhysicsPipelineLayout);
    PhysicsPipeline = LogicalDevice.createComputePipeline(nullptr, cpci);
    spdlog::info("tPhysics: Compute pipeline created");
}

void tPhysics::createShaderModules()
{
    spdlog::info("tPhysics: Creating shader module...");
    PhysicsShader = loadShaderModule(LogicalDevice, "forceNaive.comp.spv");
    spdlog::info("tPhysics: Shader modules created");
}
