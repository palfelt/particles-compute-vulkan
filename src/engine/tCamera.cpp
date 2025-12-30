#include "engine/tCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <tracy/Tracy.hpp>

#include "helpers/createBuffer.h"

tCamera::tCamera(const tVulkanDevice &device, const vk::Extent2D extent)
    : Device(device), LogicalDevice(Device.getLogicalDevice()), PhysicalDevice(device.getPhysicalDevice())
{
    spdlog::info("tCamera: Initializing...");
    createCameraBuffer();
    createDescriptorLayout();
    createDescriptorSet();
    updateDescriptorSet();

    updateProj(extent);
    updateViewData();
    spdlog::info("tCamera: Initialized");
}

void tCamera::updateViewData()
{
    ZoneScopedN("tCamera: updateViewData()");
    spdlog::trace("tCamera: Updating camera UBO...");
    updateView();
    std::memcpy(MappedCameraData, &CachedUBO, BufferSize);
    spdlog::trace("tCamera: Camera UBO updated");
}

void tCamera::addYawPitch(float deltaYaw, float deltaPitch)
{
    Yaw += deltaYaw;
    Pitch += deltaPitch;

    const float pitchLimit = glm::radians(89.0f);
    Pitch = glm::clamp(Pitch, -pitchLimit, pitchLimit);
}

void tCamera::moveLocal(const glm::vec3 &dir, float distance)
{
    // dir is in camera space: (0,0,-1) forward, (1,0,0) right, (0,1,0) up
    glm::vec3 forward = glm::normalize(glm::vec3{cos(Pitch) * sin(Yaw), sin(Pitch), cos(Pitch) * cos(Yaw)});
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    Position += forward * dir.z * distance;
    Position += right * dir.x * distance;
    Position += up * dir.y * distance;
}

void tCamera::onResize(const vk::Extent2D extent)
{
    spdlog::info("tCamera: Resizing...");
    updateProj(extent);
    spdlog::info("tCamera: Resized");
}

void tCamera::createCameraBuffer()
{
    spdlog::info("tCamera: Creating camera buffer...");
    std::tie(CameraBuffer, CameraMemory, MappedCameraData) =
        createBuffer(Device,
                     sizeof(tParams),
                     vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::SharingMode::eExclusive,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     nullptr);

    spdlog::info("tCamera: Buffer created");
}

void tCamera::createDescriptorSet()
{
    spdlog::info("tCamera: Creating descriptor set...");
    vk::DescriptorPoolSize poolSize{vk::DescriptorType::eUniformBuffer, 1};
    vk::DescriptorPoolCreateInfo poolInfo{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    DescriptorPool = LogicalDevice.createDescriptorPool(poolInfo);

    vk::DescriptorSetAllocateInfo allocInfo{*DescriptorPool, 1, &*DescriptorLayout};
    DescriptorSet = std::move(LogicalDevice.allocateDescriptorSets(allocInfo).front());
    spdlog::info("tCamera: Descriptor set created");
}

void tCamera::createDescriptorLayout()
{
    spdlog::info("tCamera: Creating descriptor set layout...");
    vk::DescriptorSetLayoutBinding cameraBinding{
        0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eMeshEXT};

    vk::DescriptorSetLayoutCreateInfo layoutInfo({}, cameraBinding);
    DescriptorLayout = LogicalDevice.createDescriptorSetLayout(layoutInfo);
    spdlog::info("tCamera: Descriptor set layout created");
}

void tCamera::updateDescriptorSet()
{
    spdlog::info("tCamera: Updating descriptor sets...");
    vk::DescriptorBufferInfo bufferInfo{*CameraBuffer, 0, BufferSize};
    vk::WriteDescriptorSet write{*DescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo};
    LogicalDevice.updateDescriptorSets(write, {});
    spdlog::info("tCamera: Descriptor set updated");
}

void tCamera::updateView()
{
    glm::vec3 forward = glm::normalize(glm::vec3{cos(Pitch) * sin(Yaw), sin(Pitch), cos(Pitch) * cos(Yaw)});
    glm::vec3 target = Position + forward;
    CachedUBO.View = glm::lookAt(Position, target, glm::vec3(0, 1, 0));
}
void tCamera::updateProj(const vk::Extent2D &extent)
{
    const float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    glm::mat4 proj = glm::perspective(FovY, aspectRatio, ZNear, ZFar);
    proj[1][1] *= -1.0f; // Vulkan Y flip
    CachedUBO.Projection = proj;
}
