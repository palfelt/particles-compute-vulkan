#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

vk::MemoryAllocateInfo getMemoryAllocateInfo(const vk::raii::PhysicalDevice &PhysicalDevice,
                                             vk::MemoryRequirements memoryRequirements,
                                             vk::MemoryPropertyFlags requirementsMask,
                                             const void *pNext = nullptr);
