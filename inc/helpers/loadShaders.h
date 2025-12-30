#pragma once

#include <string>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

vk::raii::ShaderModule loadShaderModule(const vk::raii::Device &device, const std::string &fileName);