#include "helpers/createBuffer.h"

#include "engine/tVulkanDevice.h"
#include "helpers/memoryAllocation.h"

namespace
{
void stageInitialData(const tVulkanDevice &device,
                      const vk::raii::PhysicalDevice &physicalDevice,
                      const vk::raii::Device &logicalDevice,
                      vk::raii::Buffer &destinationBuffer,
                      const vk::DeviceSize bufferSize,
                      const void *data)
{
    if (data == nullptr)
    {
        return;
    }

    spdlog::trace("createBuffer(): Creating staging buffer for initial data...");
    vk::BufferCreateInfo stagingInfo(
        {}, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
    vk::raii::Buffer stagingBuffer{logicalDevice, stagingInfo};
    const auto stagingMemReqs = stagingBuffer.getMemoryRequirements();
    const auto stagingAllocInfo =
        getMemoryAllocateInfo(physicalDevice,
                              stagingMemReqs,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    vk::raii::DeviceMemory stagingMemory{logicalDevice, stagingAllocInfo};
    stagingBuffer.bindMemory(*stagingMemory, 0);

    void *stagingMapped = stagingMemory.mapMemory(0, bufferSize);
    std::memcpy(stagingMapped, data, static_cast<size_t>(bufferSize));

    // The mapped staging memory is always released after the upload to avoid confusion.
    stagingMemory.unmapMemory();

    auto commandBuffer = device.beginSingleTimeCommands();
    vk::BufferCopy copyRegion{0, 0, bufferSize};
    commandBuffer.copyBuffer(*stagingBuffer, *destinationBuffer, copyRegion);
    device.endSingleTimeCommands(commandBuffer);
    spdlog::trace("createBuffer(): Device-local buffer created with staged data");
}
} // namespace

std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory, void *>
createBuffer(const tVulkanDevice &device,
             const vk::DeviceSize bufferSize,
             const vk::BufferUsageFlags usageFlags,
             const vk::SharingMode sharingMode,
             const vk::MemoryPropertyFlags memoryPropertyFlags,
             const void *data)
{
    spdlog::trace("createBuffer(): Creating buffer + memory...");
    const auto &logicalDevice = device.getLogicalDevice();
    const auto &physicalDevice = device.getPhysicalDevice();

    vk::BufferCreateInfo bci({}, bufferSize, usageFlags, sharingMode);
    vk::raii::Buffer buffer{logicalDevice, bci};

    const auto memReqs = buffer.getMemoryRequirements();
    vk::MemoryAllocateFlagsInfo allocFlags{};
    const bool needsDeviceAddress = static_cast<bool>(usageFlags & vk::BufferUsageFlagBits::eShaderDeviceAddress);
    if (needsDeviceAddress)
    {
        allocFlags.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
    }
    const auto allocInfo =
        getMemoryAllocateInfo(physicalDevice, memReqs, memoryPropertyFlags, needsDeviceAddress ? &allocFlags : nullptr);
    vk::raii::DeviceMemory memory{logicalDevice, allocInfo};
    buffer.bindMemory(*memory, 0);

    // Will be returned; only non-null if final memory is host-visible and kept mapped
    void *mappedPtr = nullptr;
    const bool isHostVisible = static_cast<bool>(memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);
    if (isHostVisible)
    {
        mappedPtr = memory.mapMemory(0, bufferSize);
        if (data != nullptr)
        {
            std::memcpy(mappedPtr, data, static_cast<size_t>(bufferSize));
        }

        spdlog::trace("createBuffer(): Created host-visible buffer{}",
                      data ? " with initial data" : " without initial data");
        return std::make_tuple(std::move(buffer), std::move(memory), mappedPtr);
    }

    if (data != nullptr)
    {
        stageInitialData(device, physicalDevice, logicalDevice, buffer, bufferSize, data);
    }
    else
    {
        spdlog::trace("createBuffer(): Device-local buffer created without initial data");
    }

    // For device-local memory we can't map the final buffer, so mappedPtr stays nullptr
    return std::make_tuple(std::move(buffer), std::move(memory), mappedPtr);
}
