#include "helpers/memoryAllocation.h"

vk::MemoryAllocateInfo getMemoryAllocateInfo(const vk::raii::PhysicalDevice &PhysicalDevice,
                                             vk::MemoryRequirements memoryRequirements,
                                             vk::MemoryPropertyFlags requirementsMask,
                                             const void *pNext)
{
    auto memoryTypeBits = memoryRequirements.memoryTypeBits;
    const auto memoryProperties = PhysicalDevice.getMemoryProperties();
    uint32_t ixType = UINT32_MAX;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((memoryTypeBits & 1) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask))
        {
            ixType = i;
            break;
        }
        memoryTypeBits >>= 1;
    }

    return {memoryRequirements.size, ixType, pNext};
}
