#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class tVulkanDevice;

std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory, void *>
createBuffer(const tVulkanDevice &device,
             const vk::DeviceSize bufferSize,
             const vk::BufferUsageFlags usageFlags,
             const vk::SharingMode sharingMode,
             const vk::MemoryPropertyFlags memoryPropertyFlags,
             const void *data);